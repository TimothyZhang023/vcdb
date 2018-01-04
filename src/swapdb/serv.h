//
// Created by zts on 12/18/17.
//

#ifndef VCDB_SERV_H
#define VCDB_SERV_H

#include <unordered_map>

#include "proc_scan.h"

#include "ssdb/ssdb_impl.h"
#include "common/ClientContext.hpp"
#include "common/Response.h"
#include "common/Request.h"

#include "codec/util.h"
#include "codec/internal_error.h"
#include "codec/error.h"
#include "util/bytes.h"
#include "util/strings.h"
#include "util/time.h"

#include <climits>


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


class VcServer {
public:
    VcServer(SSDB *ssdb) {
        this->db = (SSDBImpl *) ssdb;
        this->status = true;
    }

    ~VcServer() = default;

    SSDBImpl *db = nullptr;

    atomic<bool> status;

};

#endif //VCDB_SERV_H
