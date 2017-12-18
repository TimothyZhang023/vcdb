//
// Created by zts on 12/18/17.
//

#ifndef VCDB_SERV_H
#define VCDB_SERV_H

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


#define CHECK_NUM_PARAMS(n) do{ \
		if(req.size() < n){ \
			resp->push_back("client_error"); \
			resp->push_back("ERR wrong number of arguments"); \
			return 0; \
		} \
	}while(0)

const static int MAX_PACKET_SIZE = 128 * 1024 * 1024;


static const double ZSET_SCORE_MAX = std::numeric_limits<double>::max();
static const double ZSET_SCORE_MIN = std::numeric_limits<double>::min();
static const double eps = 1e-15;

static inline double millitime(){
    struct timeval now;
    gettimeofday(&now, NULL);
    double ret = now.tv_sec + now.tv_usec/1000.0/1000.0;
    return ret;
}

static inline int64_t time_ms(){
    struct timeval now;
    gettimeofday(&now, NULL);
    return (int64_t)now.tv_sec * 1000 + (int64_t)now.tv_usec/1000;
}



class SSDBServer
{
public:
    SSDBServer(SSDB *ssdb);
    ~SSDBServer();

    SSDBImpl *ssdb = nullptr;

};

class ScanParams {
public:
	std::string pattern = "*";
	uint64_t limit = 10;
};


#define reply_err_return(n) resp->reply_errror(GetErrorInfo(n)); return 0
#define reply_errinfo_return(c) resp->reply_errror((c)); return 0

typedef std::vector<Bytes> Request;





inline int prepareForScanParams(std::vector<Bytes> req, int startIndex,  ScanParams &scanParams) {

	std::vector<Bytes>::const_iterator it = req.begin() + startIndex;
	for(; it != req.end(); it += 2){
		std::string key = (*it).String();
		strtolower(&key);

		if (key=="match") {
			scanParams.pattern = (*(it+1)).String();
		} else if (key=="count") {
			scanParams.limit =  (*(it+1)).Uint64();
			if (errno == EINVAL){
				return INVALID_INT;
			}
		} else {
			return SYNTAX_ERR;
		}
	}

	return 0;
}


#endif //VCDB_SERV_H
