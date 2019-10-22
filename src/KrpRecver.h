/*
** KrpRecver.h for redis-protocol in /home/pi/redis-protocol
**
** Made by solidest
** Login   <>
**
** Started on  Tue Oct 22 1:16:38 PM 2019 solidest
** Last update Wed Oct 22 2:59:56 PM 2019 solidest
*/

#ifndef _KRPRECVER_H_
# define _KRPRECVER_H_



# define REQUEST_TYPE_UNKNOW    0
# define REQUEST_TYPE_TELNET    1
# define REQUEST_TYPE_CLIENT    2

#include <deque>
#include <vector>
#include "sds.h"
#include "KrpCommands.h"

using namespace std;

class KrpRecver
{
private:

    KrpCommands & _commands;

    int _reqtype;                    /* Request protocol type: REQUEST_TYPE_* */

    /* query */
    sds _querybuf;                   /* Buffer we use to accumulate client queries. */
    size_t _qb_pos;                  /* The position we have read in querybuf. */

    /* query parser */
    int _next_len;                   /* Next arg len */
    int _next_count;                 /* Next arg count */               

    /* Current command */
    vector<sds> _args;

    /* last error info */
    sds _lasterr;
    

public:
    KrpRecver(KrpCommands& commands);
    ~KrpRecver();
    int Feed(sds buff);
    sds GetLastErrorInfo();

private:
    void ReadCommand();
    bool ReadCmdSegment(int stopPos);
    bool ReadCmdSegmentStr(int stopPos);
    bool ReadCmdSegmentInt(int stopPos, int* len);
    void ReadCmdTelnet(int stopPos);
    void SetError(const char* err);
    char* FindNL(char* l, int len);
};

#endif /* !_KRPRECVER_H_ */
