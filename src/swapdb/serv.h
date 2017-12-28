//
// Created by zts on 12/18/17.
//

#ifndef VCDB_SERV_H
#define VCDB_SERV_H

#include <unordered_map>

#include "proc_scan.h"
#include "proc.h"

#include "ssdb/ssdb_impl.h"
#include "common/context.hpp"
#include "codec/decode.h"
#include "codec/encode.h"
#include "codec/util.h"
#include "codec/internal_error.h"
#include "codec/error.h"
#include "net/resp.h"
#include "net/redis/reponse_redis.h"
#include "util/bytes.h"
#include "util/strings.h"

#include <climits>



#define DEF_PROC(f) int proc_##f(Context &ctx, const Request &req, Response *resp)

#define CHECK_MIN_PARAMS(n) do{ \
        if(req.size() < (n)){ \
            resp->addReplyError("ERR wrong number of arguments"); \
            return 0; \
        } \
    }while(0)

#define CHECK_MAX_PARAMS(n) do{ \
        if(req.size() > (n)){ \
            resp->addReplyError("ERR wrong number of arguments"); \
            return 0; \
        } \
    }while(0)

const static int MAX_PACKET_SIZE = 128 * 1024 * 1024;


static inline int64_t time_ms() {
    struct timeval now;
    gettimeofday(&now, nullptr);
    return (int64_t) now.tv_sec * 1000 + (int64_t) now.tv_usec / 1000;
}



class VcServer {
public:
    VcServer(SSDB *ssdb) {
        this->db = (SSDBImpl *) ssdb;
        this->regProcs();
        this->status = true;
    }

    ~VcServer() {

    }

    SSDBImpl *db = nullptr;
    ProcMap procMap;

    atomic<bool> status;

    void regProcs();

};

const double ZSET_SCORE_MAX = std::numeric_limits<double>::max();
const double ZSET_SCORE_MIN = std::numeric_limits<double>::min();
const double eps = 1e-15;

#endif //VCDB_SERV_H
