
/*
**
** KrpRecver.cpp
**
*/

#include <stdio.h>
#include <assert.h>
#include "KrpRecver.h"


KrpRecver::KrpRecver(KrpCommands& commands):_args(), _commands(commands)  {

    _reqtype = REQUEST_TYPE_UNKNOW;

    /* query */
    _querybuf = sdsempty();
    _qb_pos = 0;

    /* query parser */
    _next_len = -1;
    _next_count = -1;

    /* last error info */
    _lasterr = NULL;
}

KrpRecver::~KrpRecver() {
    sdsfree(_querybuf);
    while(!_args.empty()) {
        sdsfree(_args.back());
        _args.pop_back();
    }
}

//填充接收到的buff 成功返回0 有解析错误返回-1
int KrpRecver::Feed(sds buff) {

    if(_lasterr!=NULL) {
        sdsfree(_lasterr);
        _lasterr = NULL;
    }

    if(_reqtype == REQUEST_TYPE_UNKNOW) {
        _reqtype = (buff[0] == '*' ? REQUEST_TYPE_CLIENT : REQUEST_TYPE_TELNET);
    }
        
    _querybuf = sdscatsds(_querybuf, buff);

    auto len = sdslen(_querybuf);
    if(_reqtype == REQUEST_TYPE_CLIENT) {
        while(ReadCmdSegment(len));
    }
    else {
        ReadCmdTelnet(len);
    }

    if(_qb_pos == len) {
        _qb_pos = 0;
        if(len>1024) {
            sdsfree(_querybuf);
            _querybuf = sdsempty();
        } else {
            sdsclear(_querybuf);
        }
    }

    return (_lasterr==NULL) ? 0 : -1;
}


//读取telnet命令
void KrpRecver::ReadCmdTelnet(int stopPos) {
    
    if(stopPos>10000) {
        SetError("bad telnet command format");
        return;
    }

    auto l = _querybuf + _qb_pos;
    char* p = FindNL(l, stopPos-_qb_pos);

    while(p!=NULL) {

        p[0] = '\0';
        int argc = 0;
        auto args = sdssplitargs(l, &argc);

        if(args!=NULL) {
            if(argc>0) {
                vector<sds> ci;
                for(int i = 0; i < argc; i++) {
                    ci.push_back(args[i]);
                }
                _commands.ExecuteCommand(ci);
            }
            sdsfreesplitres(args, argc);
        }
        else {
            this->SetError("bad command format");
        }
        
        _qb_pos += (p-l+2);

        if(_qb_pos == stopPos)
            break;
        l = _querybuf + _qb_pos;
        p = FindNL(l, stopPos-_qb_pos+1);
    }
}


//设置异常信息
void KrpRecver::SetError(const char* err) {
    _lasterr = sdsnew(err);
}

//最后的错误信息
sds KrpRecver::GetLastErrorInfo() {
    return _lasterr;
}

//Find NL
char* KrpRecver::FindNL(char* l, int len) {
    char* p = l;
    int pos = 1;  

    while(pos<len) {
        if(l[pos-1]=='\r' && l[pos]=='\n')
            return &l[pos-1];
        pos++;
    }
    return NULL;
}


//读取任意一段
bool KrpRecver::ReadCmdSegment(int stopPos) {

    if(_next_len>0)
        return ReadCmdSegmentStr(stopPos) ? (_qb_pos != stopPos) : false;

    char * p = _querybuf + _qb_pos;
    switch (*p) {

        case '*': {
            if(_next_count>0) {
                SetError("bad command format1");
                return false;
            }
            int ii = -1;
            if(ReadCmdSegmentInt(stopPos, &ii)) {
                _next_count = ii;
                return _qb_pos != stopPos;
            }
            return false;
        }

        case '$': {
            if(_next_count<0) {
                SetError("bad command format3");
                return false;
            }
            int ii = -1;
            if(ReadCmdSegmentInt(stopPos, &ii)) {
                _next_len = ii;
                return _qb_pos != stopPos;
            }
            return false;
        }

        case '\n':
        case '\r':
            return ++_qb_pos != stopPos;
    
        default:
            SetError("bad command format6");
            return false;
            break;
    }
}

//读取数字字段
bool KrpRecver::ReadCmdSegmentInt(int stopPos, int* len) {
    int pos = _qb_pos + 1;
    if(pos + 2 > stopPos)
        return false;
    int reti = 0;
    auto p = _querybuf + pos;

    while(*p!='\r') {
        reti = (reti*10) + (*p-'0');
        //已经到达尾部
        if(++pos == stopPos)
            return false;
        p++;
    }

    if(reti <= 0) {
        SetError("bad number");
        return false;
    }

    if(stopPos>pos && stopPos>++pos) {
        pos++;
    }
    _qb_pos = pos;
    *len = reti;
    return true;
}

//读取字符串段
bool KrpRecver::ReadCmdSegmentStr(int stopPos) {
    if(_qb_pos + _next_len > stopPos)   //还不够数
        return false;
    auto s = sdsempty();
    s = sdscatlen(s, _querybuf+_qb_pos, _next_len);

    if(_next_count<0) {
        SetError("bad command format7");
        return false;
    }
    else {
        _args.push_back(s);
        if(_args.size()==_next_count)
        {
            _commands.ExecuteCommand(_args);
            while(!_args.empty()) {
                sdsfree(_args.back());
                _args.pop_back();
            }
            _next_count = -1;
        }
    }

    _qb_pos += _next_len;
    _next_len = -1;
    if(stopPos>_qb_pos && stopPos>++_qb_pos) {
        _qb_pos++;
    }
    return true;
}
