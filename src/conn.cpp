//
// Created by zts on 12/18/17.
//
#include "slash/include/slash_string.h"
#include "slash/include/env.h"

#include "conn.h"

#include <util/mem.h>


static std::map<std::string, std::string> db;


VcClientConn::VcClientConn(int fd, const std::string &ip_port, pink::ServerThread *thread, void *worker_specific_data)
        : RedisConn(fd, ip_port, thread) {
    // Handle worker_specific_data ...
}

VcClientConn::~VcClientConn() {
}

int VcClientConn::DealMessage() {
    if (argv_.empty()) return -2;

    std::string opt = argv_[0];
    slash::StringToLower(opt);


    printf("Get redis message %s ", hexcstr(opt));


    for (int i = 0; i < argv_.size(); i++) {
        printf("%s ", hexcstr(argv_[i]));
    }
    printf("\n");

    uint64_t start_us = slash::NowMicros();



    std::string val = "result";
    std::string res;
    // set command
    if (argv_.size() == 3) {
        res = "+OK\r\n";
        db[argv_[1]] = argv_[2];
        memcpy(wbuf_ + wbuf_len_, res.data(), res.size());
        wbuf_len_ += res.size();
    } else {
        std::map<std::string, std::string>::iterator iter = db.find(argv_[1]);
        if (iter != db.end()) {
            val = iter->second;
            memcpy(wbuf_ + wbuf_len_, "*1\r\n$", 5);
            wbuf_len_ += 5;
            std::string len = std::to_string(val.length());
            memcpy(wbuf_ + wbuf_len_, len.data(), len.size());
            wbuf_len_ += len.size();
            memcpy(wbuf_ + wbuf_len_, "\r\n", 2);
            wbuf_len_ += 2;
            memcpy(wbuf_ + wbuf_len_, val.data(), val.size());
            wbuf_len_ += val.size();
            memcpy(wbuf_ + wbuf_len_, "\r\n", 2);
            wbuf_len_ += 2;
        } else {
            res = "$-1\r\n";
            memcpy(wbuf_ + wbuf_len_, res.data(), res.size());
            wbuf_len_ += res.size();
        }
    }

    set_is_reply(true);


    int64_t duration = slash::NowMicros() - start_us;


    return 0;
}


