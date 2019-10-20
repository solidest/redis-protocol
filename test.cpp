
#include <stdio.h>
#include <string.h>

static int tests = 0, fails = 0;
#define test(_s) { printf("#%02d ", ++tests); printf(_s); }
#define test_cond(_c) if(_c) printf("\033[0;32mPASSED\033[0;0m\n"); else {printf("\033[0;31mFAILED\033[0;0m\n"); fails++;}

int main(int argc, char **argv) {
    int res;
    const char *argv[3];
    argv[0] = "SET";
    argv[1] = "foo\0xxx";
    argv[2] = "bar";
    size_t lens[3] = { 3, 7, 3 };

    test("ClientWriter: ");
    ClientWriter c_writer;
    sds sds_cmd;
    int len = c_writer.FormatCmd(sds_cmd,argc,argv,lens);
    test_cond(strncmp(sds_cmd,"*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n",len) == 0 && len == 4+4+(3+2)+4+(3+2)+4+(3+2));


    test("ClientReader: ");
    ClientReader c_reader;
    Reply reply;
    res = c_reader.Feed((char*)"+OK",3, reply);
    test_cond(res==REDIS_OK && reply.IsEmpty());
    res = c_reader.Feed((char*)"\r\n",2, &reply);
    test_cond(res==REDIS_OK && reply.IsOk());

    test("ServerReader: ");
    ServerReader s_reader;
    Command cmd;
    res = s_reader.Feed((char*)"*3\r", 3, cmd);
    test_cond(res==REDIS_OK && cmd.IsEmpty());
    res = s_reader.Feed("\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n", len-3, cmd);
    test_cond(res==REDIS_OK && trcasecmp(cmd.Name(),"SET") == 0);

    test("ServerWriter: ");
    ServerWriter s_writer;
    sds sds_reply;
    // void addReplyNull(client *c);
    // void addReplyNullArray(client *c);
    // void addReplyBool(client *c, int b);
    // void addReplyVerbatim(client *c, const char *s, size_t len, const char *ext);
    // void addReplyProto(client *c, const char *s, size_t len);
    // void AddReplyFromClient(client *c, client *src);
    // void addReplyBulk(client *c, robj *obj);
    // void addReplyBulkCString(client *c, const char *s);
    // void addReplyBulkCBuffer(client *c, const void *p, size_t len);
    // void addReplyBulkLongLong(client *c, long long ll);
    // void addReply(client *c, robj *obj);
    // void addReplySds(client *c, sds s);
    // void addReplyBulkSds(client *c, sds s);
    // void addReplyError(client *c, const char *err);
    // void addReplyStatus(client *c, const char *status);
    // void addReplyDouble(client *c, double d);
    // void addReplyHumanLongDouble(client *c, long double d);
    // void addReplyLongLong(client *c, long long ll);
    // void addReplyArrayLen(client *c, long length);
    // void addReplyMapLen(client *c, long length);
    // void addReplySetLen(client *c, long length);
    // void addReplyAttributeLen(client *c, long length);
    // void addReplyPushLen(client *c, long length);
    // void addReplyHelp(client *c, const char **help);
    // void addReplySubcommandSyntaxError(client *c);
    // void addReplyLoadedModules(client *c);
    sds_cmd.Empty();
    s_writer.addReplyBulkCString(sds_reply, "ok");
    test_cond(strcasecmp(sds_reply.StringValue(),"+ok") == 0);

    test("Client && Server: ");
    Stream st;
    KServer server(st);
    server.CreateChannel(st);

    KClient client(st);
    reply.Empty();
    client.EmitCommand("ping", reply);
    test_cond(strcasecmp(reply.StringValue(),"pong") == 0);

    sdsfree(sds_cmd);

}
