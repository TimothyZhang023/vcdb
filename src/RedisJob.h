//
// Created by zts on 12/18/17.
//

#ifndef VCDB_REDIS_CVT_H
#define VCDB_REDIS_CVT_H

#include <string>
#include <unordered_map>
#include <vector>
#include <util/bytes.h>
#include <proc/proc_common.h>

class RedisJob {
    std::vector<std::string> recv_string;

public:
    RedisJob(const std::vector<std::string> &recv_string, std::string *response);

    Request req;
    Response response;
    std::string cmd;

    virtual ~RedisJob();
};


template<class T>
static std::string serialize_req(T &req) {
    std::string ret;
    char buf[50];
    for (int i = 0; i < req.size(); i++) {
        if (i >= 5 && i < req.size() - 1) {
            sprintf(buf, "[%d more...]", (int) req.size() - i);
            ret.append(buf);
            break;
        }
        if (((req[0] == "get" || req[0] == "set") && i == 1) || req[i].size() < 50) {
            if (req[i].size() == 0) {
                ret.append("\"\"");
            } else {
                std::string h = hexmem(req[i].data(), req[i].size());
                ret.append(h);
            }
        } else {
            sprintf(buf, "[%d]", (int) req[i].size());
            ret.append(buf);
        }
        if (i < req.size() - 1) {
            ret.append(" ");
        }
    }
    return ret;
}

#endif //VCDB_REDIS_CVT_H
