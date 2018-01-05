//
// Created by zts on 12/18/17.
//

#ifndef VCDB_SERV_H
#define VCDB_SERV_H

#include <unordered_map>
#include <climits>

#include "storage/ssdb_impl.h"

#include "common/ClientContext.hpp"
#include "common/Response.h"
#include "common/Request.h"

#include "codec/util.h"
#include "codec/internal_error.h"
#include "codec/error.h"

#include "util/bytes.h"
#include "util/strings.h"
#include "util/time.h"


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


class ScanParams {
public:
    std::string pattern = "*";
    uint64_t limit = 10;
};


inline int prepareForScanParams(std::vector<Bytes> req, int startIndex, ScanParams &scanParams) {

    std::vector<Bytes>::const_iterator it = req.begin() + startIndex;
    for (; it != req.end(); it += 2) {
        std::string key = (*it).String();
        strtolower(&key);

        if (key == "match") {
            scanParams.pattern = (*(it + 1)).String();
        } else if (key == "count") {
            scanParams.limit = (*(it + 1)).Uint64();
            if (errno == EINVAL) {
                return INVALID_INT;
            }
        } else {
            return SYNTAX_ERR;
        }
    }

    return 0;
}

#endif //VCDB_SERV_H
