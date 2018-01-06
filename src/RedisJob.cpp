//
// Created by zts on 12/18/17.
//

#include "RedisJob.h"

RedisJob::RedisJob(const std::vector<std::string> &recv_string, std::string *res) :
        recv_string(recv_string), response(Response(res)) {
    for (const auto &t : recv_string) {
        req.emplace_back(t);
    }
    cmd = recv_string.at(0);
}

