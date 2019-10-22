/*
** ClientWriter.h for redis-protocol in /home/pi/redis-protocol
**
** Made by solidest
** Login   <solidest>
**
** Started on  undefined Oct 20 3:18:01 PM 2019 solidest
** Last update Wed Oct 22 1:15:09 PM 2019 solidest
*/

#ifndef _CLIENTWRITER_H_
# define _CLIENTWRITER_H_


#include "SdsWrapper.h"


#define KRP_ERR -1
#define KRP_OK 0

typedef void SendHandle(sds buf);

class KrpSender {

    public:
        KrpSender(SendHandle* psender);
        int FormatCommand(SdsWrapper& target, int argc, const char ** argv, const size_t *argvlen);
        void SendCommand(int argc, const char ** argv, const size_t *argvlen);

    private:
        SendHandle* _psender;
};


#endif /* !_CLIENTWRITER_H_ */
