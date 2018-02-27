//
// Created by zts on 1/5/18.
//

#ifndef VCDB_BINLOG_H
#define VCDB_BINLOG_H


#include <storage/ssdb_impl.h>

namespace vcdb {

    class Binlog {
    public:
        RecordKeyMutex mutex_record_;
        int Put(const std::string &content);
    private:

    };

}


#endif //VCDB_BINLOG_H
