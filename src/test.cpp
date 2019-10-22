
#include <stdio.h>
#include <string.h>
#include <vector>

#include "KrpSender.h"
#include "KrpRecver.h"
#include "KrpCommands.h"
#include "Krp.h"

static int tests = 0, fails = 0;
#define test(_s) { printf("#%02d ", ++tests); printf(_s); }
#define test_cond(_c) if(_c) printf("\033[0;32mPASSED\033[0;0m\n"); else {printf("\033[0;31mFAILED\033[0;0m\n"); fails++;}


void CommandDispatcher(vector<sds>& args) {
    if(args.size()>0) {
        printf("ExecCommand(%s", args[0]);
        for(int i=1; i<args.size(); i++) {
            printf(" %s", args[i]);
        }
        printf(") ");       
    }

    test_cond(args.size()>=1);
}

void SendBuffer(sds buf) {
    // printf("%s", buf);
    test_cond(buf[0]=='*');
}

int main(int argcs, char **argvs) {

    KrpCommands::InitialCommands();

    int len;
    int res;
    const char *argv[3];
    argv[0] = "SET";
    argv[1] = "foo\0xxx";
    argv[2] = "bar";
    size_t lens[3] = { 3, 7, 3 };
    int argc = 3;

    KrpSender sender(SendBuffer);
    KrpCommands cmds(sender);
    KrpRecver recver(cmds);

    sds sds_cmd;
    test("Format command into sds by passing argc/argv without lengths: ");
    len = sender.FormatCommand(&sds_cmd,argc,argv,NULL);
    test_cond(strncmp(sds_cmd,"*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n",len) == 0 &&
        len == 4+4+(3+2)+4+(3+2)+4+(3+2));
    sdsfree(sds_cmd);

    test("Format command into sds by passing argc/argv with lengths: ");
    len = sender.FormatCommand(&sds_cmd,argc,argv,lens);
    test_cond(strncmp(sds_cmd,"*3\r\n$3\r\nSET\r\n$7\r\nfoo\0xxx\r\n$3\r\nbar\r\n",len) == 0 &&
        len == 4+4+(3+2)+4+(7+2)+4+(3+2));
    sdsfree(sds_cmd);

    test("Feed client buffer to recver: ");
    
    sds buf1 = sdsnew("*3\r");
    sds buf2 = sdsnew("\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n");
    res = recver.Feed(buf1);
    res = recver.Feed(buf2);
    sdsfree(buf1);
    sdsfree(buf2);

    test("Feed telnet buffer to recver: ");
    KrpRecver recver2(cmds);
    sds buf3 = sdsnew("ping\r\n");
    recver2.Feed(buf3);
    sdsfree(buf3);

    test("Format from Sender then Feed to Recver: ");
    KrpSender senderx(SendBuffer);
    KrpCommands cmdsx(sender);
    KrpRecver recverx(cmds);
    senderx.FormatCommand(&sds_cmd,argc,argv,lens);
    recverx.Feed(sds_cmd);
    sdsfree(sds_cmd);

    test("Krp: ");
    Krp krp(SendBuffer);
    sds buf = sdsnew("*1\r\n$4\r\nping\r\n");
    krp.Recved(buf);
    sdsfree(buf);
}
