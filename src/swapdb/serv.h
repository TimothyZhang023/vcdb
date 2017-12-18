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

#define DEF_PROC(f) int proc_##f(Context &ctx, const Request &req, Response *resp)

#define CHECK_MIN_PARAMS(n) do{ \
		if(req.size() < n){ \
			resp->push_back("client_error"); \
			resp->push_back("ERR wrong number of arguments"); \
			return 0; \
		} \
	}while(0)

#define CHECK_MAX_PARAMS(n) do{ \
		if(req.size() > n){ \
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

typedef std::vector<Bytes> Request;


typedef int (*proc_t)(Context &ctx, const Request &req, Response *resp);

struct Command{
    static const int FLAG_READ		= (1 << 0);
    static const int FLAG_WRITE		= (1 << 1);
    static const int FLAG_BACKEND	= (1 << 2);
    static const int FLAG_THREAD	= (1 << 3);

    std::string name;
    int flags;
    proc_t proc;
    uint64_t calls;
    double time_wait;
    double time_proc;

    Command(){
        flags = 0;
        proc = nullptr;
        calls = 0;
        time_wait = 0;
        time_proc = 0;
    }
};



struct BytesEqual{
    bool operator()(const Bytes &s1, const Bytes &s2) const {
        return (bool)(s1.compare(s2) == 0);
    }
};
struct BytesHash{
    size_t operator()(const Bytes &s1) const {
        unsigned long __h = 0;
        const char *p = s1.data();
        for (int i=0 ; i<s1.size(); i++)
            __h = 5*__h + p[i];
        return size_t(__h);
    }
};

#define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#if GCC_VERSION >= 403
#include <tr1/unordered_map>
typedef std::tr1::unordered_map<Bytes, Command *, BytesHash, BytesEqual> proc_map_t;
#else
#ifdef __clang__
		#include <unordered_map>
		typedef std::unordered_map<Bytes, Command *, BytesHash, BytesEqual> proc_map_t;
	#else
		#include <ext/hash_map>
		typedef __gnu_cxx::hash_map<Bytes, Command *, BytesHash, BytesEqual> proc_map_t;
	#endif
#endif


class ProcMap
{
private:
    proc_map_t proc_map;

public:
    ProcMap();
    ~ProcMap();
    void set_proc(const std::string &cmd, const char *sflags, proc_t proc);
    void set_proc(const std::string &cmd, proc_t proc);
    Command* get_proc(const Bytes &str);

    proc_map_t::iterator begin(){
        return proc_map.begin();
    }
    proc_map_t::iterator end(){
        return proc_map.end();
    }
};


class SSDBServer
{
public:
    SSDBServer(SSDB *ssdb) {
        this->ssdb = (SSDBImpl *) ssdb;
        this->reg_procs();
    }

    ~SSDBServer();

    SSDBImpl *ssdb = nullptr;
    ProcMap proc_map;

    void reg_procs();

};

class ScanParams {
public:
	std::string pattern = "*";
	uint64_t limit = 10;
};


#define reply_err_return(n) resp->reply_errror(GetErrorInfo(n)); return 0
#define reply_errinfo_return(c) resp->reply_errror((c)); return 0






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
