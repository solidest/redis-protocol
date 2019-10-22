/*
** KrpCommands.h for redis-protocol in /home/pi/redis-protocol/src
**
** Made by solidest
** Login   <>
**
** Started on  Tue Oct 22 3:44:56 PM 2019 solidest
** Last update Tue Oct 22 3:44:56 PM 2019 solidest
*/

#ifndef _KRPCOMMANDS_H_
# define _KRPCOMMANDS_H_

#include <vector>
#include "KrpSender.h"

using namespace std;

typedef void pCommandFunc(KrpSender& sender, vector<sds>& ci);
typedef struct CommandInfo
{
    const char * name;
    pCommandFunc* command;
} CommandInfo;

class KrpCommands {

    public:
        KrpCommands(KrpSender& sender);
        static void InitialCommands();
        void ExecuteCommand(vector<sds>& callinfo);
        void ExecuteCommandError(const char* error);

    private:
        KrpSender& _sender;
};


#endif /* !_KRPCOMMANDS_H_ */
