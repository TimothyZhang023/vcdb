/*
 * This file is react from pika server
 *  The MIT License (MIT)

Copyright © 2018 <Qihoo360>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


 */

#ifndef VCDB_BINLOGMANAGER_H
#define VCDB_BINLOGMANAGER_H

//class Binlog;

#include <cstdio>
#include <string>
#include <map>
#include <vector>
#include <slash/include/slash_string.h>

#include "pika/pika_define.h"
#include "pika/pika_binlog.h"

class BinlogManager {

    Binlog *logger_;

    std::string log_path;
    uint64_t expire_logs_nums = 3;
    int64_t expire_logs_days = 0;


public:
    BinlogManager(Binlog *logger_, const string &log_path, int64_t expire_logs_nums, int64_t expire_logs_days);

    bool PurgeFiles(uint32_t to, bool manual, bool force);

private:


    bool GetBinlogFiles(std::map<uint32_t, std::string> &binlogs);

    bool CouldPurge(uint32_t index);

};


#endif //VCDB_BINLOGMANAGER_H
