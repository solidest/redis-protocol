/*
** ClientWriter.h for redis-protocol in /home/pi/redis-protocol
**
** Made by solidest
** Login   <solidest>
**
** Started on  undefined Oct 20 3:18:01 PM 2019 solidest
** Last update undefined Oct 20 3:18:01 PM 2019 solidest
*/

#ifndef _CLIENTWRITER_H_
# define _CLIENTWRITER_H_


#include "sds.h"


#define KRP_ERR -1
#define KRP_OK 0

class KrpClient {

    public:
        // int redisvFormatCommand(char **target, const char *format, va_list ap);
        // int redisFormatCommand(char **target, const char *format, ...);
        // _int redisFormatCommandArgv(char **target, int argc, const char **argv, const size_t *argvlen);
        // int redisFormatSdsCommandArgv(sds *target, int argc, const char ** argv, const size_t *argvlen);
        // void redisFreeCommand(char *cmd);
        // void redisFreeSdsCommand(sds cmd);

        int FormatCommand(sds *target, int argc, const char ** argv, const size_t *argvlen);
        void FreeCommand(sds cmd);

};


#endif /* !_CLIENTWRITER_H_ */
