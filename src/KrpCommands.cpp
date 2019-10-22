


#include <unordered_map>

#include "KrpCommands.h"

void ping(KrpSender& sender, vector<sds>& ci) {
    const char *argv[1] = { "pong" };
    sender.SendCommand(1, argv, NULL);
}


unordered_map<string, CommandInfo*> __cmdsTable;
struct CommandInfo cmdTable[] = {
    {"ping", ping },
};

//initial commands
void KrpCommands::InitialCommands()
{
    int numcommands = sizeof(__cmdsTable)/sizeof(struct CommandInfo);
    for(int i = 0; i < numcommands; i++) {
        struct CommandInfo *cmdi = cmdTable+i;
        __cmdsTable.insert(pair<string, CommandInfo*>(cmdi->name, cmdi));
    }
}

KrpCommands::KrpCommands(KrpSender& sender):_sender(sender) {

}

//out error info
void KrpCommands::ExecuteCommandError(const char* error) {
    const char *argv[2] = { "error", error};
    this->_sender.SendCommand(2, argv, NULL);
}

//执行命令
void KrpCommands::ExecuteCommand(vector<sds>& callinfo) {
    auto cmdi = __cmdsTable.find(callinfo.front());
    if(cmdi != __cmdsTable.end()) {
        auto cmd = cmdi->second;
        try {
            (cmd->command)(this->_sender, callinfo);
        }
        catch(const std::exception& e) {
            ExecuteCommandError(e.what());
        }
    }
    else {
        ExecuteCommandError(callinfo.front());
    }
}