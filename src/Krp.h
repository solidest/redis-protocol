/*
** Krp.h for redis-protocol in /home/pi/redis-protocol/src
**
** Made by solidest
** Login   <>
**
** Started on  Tue Oct 22 5:55:47 PM 2019 solidest
** Last update Tue Oct 22 5:55:47 PM 2019 solidest
*/

#ifndef _KRP_H_
# define _KRP_H_

#include "KrpCommands.h"
#include "KrpRecver.h"
#include "KrpSender.h"

class Krp {
    public:
        Krp(SendHandle* hsend):_sender(hsend), _cmds(_sender), _recver(_cmds) {

        }

        void Recved(sds buf) {
            _recver.Feed(buf);
        }
        
    private:
        KrpCommands _cmds;
        KrpRecver _recver;
        KrpSender _sender;
};

#endif /* !_KRP_H_ */
