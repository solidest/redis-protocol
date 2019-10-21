/*
** EPITECH PROJECT, 2019
** redis-protocol
** File description:
** SdsWrapper
*/

#ifndef _SDSWRAPPER_H_
#define _SDSWRAPPER_H_

#include "sds.h"
#include <assert.h>

class SdsWrapper {
    public:
        SdsWrapper() {
            this->_sds = sdsempty();
        };

        void Attach(sds s) {
            assert(_sds==nullptr);
            _sds = s;
        }

        sds Dettach() {
            assert(_sds!=nullptr);
            sds res = _sds;
            _sds = nullptr;
            return res;
        }

        sds Get() {
            return _sds;
        }

        void Reset() {
            sdsfree(_sds);
            _sds = sdsempty();
        }

        ~SdsWrapper() {
            sdsfree(_sds);
        }



    private:
        sds _sds;
};


#endif /* !solidest */
