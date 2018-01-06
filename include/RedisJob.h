//
// Created by zts on 12/18/17.
//

#ifndef VCDB_REDIS_CVT_H
#define VCDB_REDIS_CVT_H

#include <string>
#include <unordered_map>
#include <vector>
#include <util/bytes.h>

#include <Request.h>
#include <Response.h>

class RedisJob {
    std::vector<std::string> recv_string;

public:
    RedisJob(const std::vector<std::string> &recv_string, std::string *response);

    Request req;
    Response response;
    std::string cmd;

    virtual ~RedisJob() = default;;
};



#endif //VCDB_REDIS_CVT_H
