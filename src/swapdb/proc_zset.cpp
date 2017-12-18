/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
/* zset */
#include "serv.h"


int proc_multi_zset(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(4);

    VcServer *serv = ctx.serv;
	int flags = ZADD_NONE;

    int elements;
	const Bytes &name = req[1];

	int scoreidx = 2;
	std::vector<Bytes>::const_iterator it = req.begin() + scoreidx;
	for(; it != req.end(); it += 1){
        std::string key = (*it).String();
        strtolower(&key);

		if (key=="nx") {
            flags |= ZADD_NX;
        } else if (key=="xx") {
            flags |= ZADD_XX;
        } else if (key=="ch") {
            flags |= ZADD_CH;
        } else if (key=="incr") {
            flags |= ZADD_INCR;
        } else
            break;
        scoreidx++;
    }

    elements = (int)req.size() - scoreidx;
    if(elements < 0){
        reply_errinfo_return("ERR wrong number of arguments for 'zadd' command");
    } else if((elements == 0) | (elements % 2 != 0)){
		//wrong args
        reply_err_return(SYNTAX_ERR);
    }
    elements /= 2;

    int incr = (flags & ZADD_INCR) != 0;
    int nx = (flags & ZADD_NX) != 0;
    int xx = (flags & ZADD_XX) != 0;

    /* XX and NX options at the same time are not compatible. */
    if (nx && xx) {
        reply_errinfo_return("ERR XX and NX options at the same time are not compatible");
    }

    if (incr){
        if (incr && elements > 1){
            reply_errinfo_return("ERR INCR option supports a single increment-element pair");
        }

        double score = req[scoreidx].Double();
        if (errno == EINVAL){
            reply_err_return(INVALID_DBL);
        }

        double new_val = 0;
        int ret = serv->db->zincr(ctx, name, req[scoreidx+1], score, flags, &new_val);

        if (ret < 0){
            reply_err_return(ret);
        }

        resp->reply_double(0, new_val);

        {
            int processed = 0;
            if (!(flags & ZADD_NOP)) processed++;
            if (processed){
                resp->redisResponse = new RedisResponse(resp->resp[1]);
            } else{
                resp->redisResponse = new RedisResponse("");
                resp->redisResponse->type = REDIS_REPLY_NIL;
            }
        }


        return 0;
    }

    std::map<Bytes,Bytes> sortedSet;

	it = req.begin() + scoreidx;
	for(; it != req.end(); it += 2){
		const Bytes &key = *(it + 1);
		const Bytes &val = *it;

//        double score = val.Double();
//        if (errno == EINVAL){
//            reply_err_return(INVALID_DBL);
//        }
//
//        if (score <= ZSET_SCORE_MIN || score >= ZSET_SCORE_MAX){
//            reply_err_return(VALUE_OUT_OF_RANGE);
//        }

        if (nx) {
            sortedSet.insert(make_pair(key,val));
        } else {
            sortedSet[key]=val;
        }
	}

    int64_t num = 0;
    int ret = serv->db->multi_zset(ctx, req[1], sortedSet, flags, &num);

    if (ret < 0){
        reply_err_return(ret);
    }

    resp->reply_int(0, num);

	return 0;
}

int proc_multi_zdel(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(3);

	const Bytes &name = req[1];
    std::set<Bytes> keys;

    for_each(req.begin() + 2, req.end(), [&](Bytes b) {
        keys.insert(b);
    });

    int64_t count = 0;
    int ret = serv->db->multi_zdel(ctx, name, keys, &count);

    if (ret < 0){
        reply_err_return(ret);
    }

	resp->reply_int(0, count);
	return 0;
}


int proc_zsize(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(2);

    uint64_t size = 0;
	int ret = serv->db->zsize(ctx, req[1], &size);

    if (ret < 0) {
        reply_err_return(ret);
    }
	resp->reply_int(ret, size);
	return 0;
}

int proc_zget(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(3);

	double score = 0;
	int ret = serv->db->zget(ctx, req[1], req[2], &score);

    if (ret < 0) {
        reply_err_return(ret);
    } else if(ret == 0){
        resp->reply_not_found();
        return 0;
    } else{
		resp->reply_double(ret, score);
	}
	return 0;
}

int proc_zrank(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(3);

    int64_t rank = 0;
	int ret = serv->db->zrank(ctx, req[1], req[2], &rank);

    if (ret < 0) {
        reply_err_return(ret);
    } else if (ret == 0 || rank == -1) {
        resp->reply_not_found();
    } else {
		resp->reply_int(ret, rank);
	}
	return 0;
}

int proc_zrrank(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(3);

    int64_t rank = 0;
	int ret = serv->db->zrrank(ctx, req[1], req[2], &rank);

    if (ret < 0) {
        reply_err_return(ret);
    } else if (ret == 0 || rank == -1) {
        resp->reply_not_found();
    } else{
		resp->reply_int(ret, rank);
	}
	return 0;
}

int proc_zrange(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(4);

    resp->reply_list_ready();
    int ret = serv->db->zrange(ctx, req[1], req[2], req[3], resp->resp);

    if (ret < 0){
        resp->resp.clear();
        reply_err_return(ret);
    }

	return 0;
}

int proc_zrrange(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(4);

    resp->reply_list_ready();
    int ret = serv->db->zrrange(ctx, req[1], req[2], req[3], resp->resp);

    if (ret < 0){
        resp->resp.clear();
        reply_err_return(ret);
    }

	return 0;
}

int string2ld(const char *s, size_t slen, long *value) {
    long long lvalue;
    if (string2ll(s,slen,&lvalue) == 0)
        return NAN_SCORE;
    if (lvalue < LONG_MIN || lvalue > LONG_MAX) {
        return NAN_SCORE;
    }
    if (value) *value = lvalue;

    return 1;
}

static int _zrangebyscore(Context &ctx, SSDB *ssdb, const Request &req, Response *resp, int reverse){
    CHECK_MIN_PARAMS(4);
    long offset = 0, limit = -1;
    int withscores = 0;
    int ret = 0;

    if (req.size() > 4) {
        int remaining = req.size() - 4;
        int pos = 4;

        while (remaining) {
            if (remaining >= 1 && !strcasecmp(req[pos].data(),"withscores")) {
                pos++; remaining--;
                withscores = 1;
            } else if (remaining >= 3 && !strcasecmp(req[pos].data(),"limit")) {
                if ( (string2ld(req[pos+1].data(),req[pos+1].size(),&offset) < 0) ||
                     (string2ld(req[pos+2].data(),req[pos+2].size(),&limit) < 0) ){
                    reply_errinfo_return("ERR min or max is not a float");
                }
                pos += 3; remaining -= 3;
            } else {
                reply_err_return(SYNTAX_ERR);
            }
        }
    }

    resp->reply_list_ready();
    if (reverse){
        ret = ssdb->zrevrangebyscore(ctx, req[1], req[2], req[3], resp->resp, withscores, offset, limit);
    } else{
        ret = ssdb->zrangebyscore(ctx, req[1], req[2], req[3], resp->resp, withscores, offset, limit);
    }

    if (ret < 0){
        resp->resp.clear();
        reply_err_return(ret);
    }

    return 0;
}

int proc_zrangebyscore(Context &ctx, const Request &req, Response *resp){
    VcServer *serv = ctx.serv;
    return _zrangebyscore(ctx, serv->db, req, resp, 0);
}

int proc_zrevrangebyscore(Context &ctx, const Request &req, Response *resp){
    VcServer *serv = ctx.serv;
    return _zrangebyscore(ctx, serv->db, req, resp, 1);
}

int proc_zscan(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(3);
    VcServer *serv = ctx.serv;


    int cursorIndex = 2;

    Bytes cursor = req[cursorIndex];

    cursor.Uint64();
    if (errno == EINVAL){
        reply_err_return(INVALID_INT);
    }

    ScanParams scanParams;

    int ret = prepareForScanParams(req, cursorIndex + 1, scanParams);
    if (ret < 0) {
        reply_err_return(ret);
    }

    resp->reply_scan_ready();

    ret =  serv->db->zscan(ctx, req[1], cursor, scanParams.pattern, scanParams.limit, resp->resp);

    if (ret < 0) {
        resp->resp.clear();
        reply_err_return(ret);
    } else if (ret == 0) {
    }

    return 0;
}


// dir := +1|-1
static int _zincr(Context &ctx, SSDB *ssdb, const Request &req, Response *resp, int dir){
	CHECK_MIN_PARAMS(4);
    CHECK_MAX_PARAMS(4);
    int flags = ZADD_NONE;
    flags |= ZADD_INCR;

    double score = req[2].Double();
    if (errno == EINVAL){
        reply_err_return(INVALID_DBL);
    }

    double new_val = 0;
    int ret = ssdb->zincr(ctx, req[1], req[3], dir * score, flags, &new_val);
    if (ret < 0){
        reply_err_return(ret);
    }

    resp->reply_double(0, new_val);

    {
        int processed = 0;
        if (!(flags & ZADD_NOP)) processed++;
        if (processed){
            resp->redisResponse = new RedisResponse(resp->resp[1]);
        } else{
            resp->redisResponse = new RedisResponse("");
            resp->redisResponse->type = REDIS_REPLY_NIL;
        }
    }

	return 0;
}

int proc_zincr(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	return _zincr(ctx, serv->db, req, resp, 1);
}

int proc_zcount(Context &ctx, const Request &req, Response *resp){
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(4);

    int64_t count = 0;
    int ret = serv->db->zremrangebyscore(ctx, req[1], req[2], req[3], false, &count);

    if (ret < 0) {
        reply_err_return(ret);
    }

    resp->reply_int(0, count);
    return 0;
}

int proc_zremrangebyscore(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(4);

 	int64_t count = 0;
    int ret = serv->db->zremrangebyscore(ctx, req[1], req[2], req[3], true, &count);

    if (ret < 0) {
		reply_err_return(ret);
	}

	resp->reply_int(0, count);
	return 0;
}

int proc_zremrangebyrank(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(4);

    std::vector<std::string> key_score;
    int ret = serv->db->zrange(ctx, req[1], req[2], req[3], key_score);

    if (ret < 0) {
        reply_err_return(ret);
    } else if (ret == 0) {
        resp->reply_int(0, ret);
        return 0;
    }

    std::set<Bytes> keys;
    for (int i = 0; i < key_score.size(); i += 2) {
        keys.insert(Bytes(key_score[i]));
    }

    int64_t count = 0;
    ret = serv->db->multi_zdel(ctx, req[1], keys, &count);

    if (ret < 0) {
        reply_err_return(ret);
    }

	resp->reply_int(0, count);

	return 0;
}


static int _zrangebylex(Context &ctx, SSDB *ssdb, const Request &req, Response *resp, enum DIRECTION direction){
    CHECK_MIN_PARAMS(4);
    long offset = 0, limit = -1;
    int ret = 0;

    if (req.size() > 4) {
        int remaining = req.size() - 4;
        int pos = 4;

        while (remaining) {
            if (remaining >= 3 && !strcasecmp(req[pos].data(),"limit")) {
                if ( (string2ld(req[pos+1].data(),req[pos+1].size(),&offset) < 0) ||
                     (string2ld(req[pos+2].data(),req[pos+2].size(),&limit) < 0) ){
                    reply_err_return(NAN_SCORE);
                }
                pos += 3; remaining -= 3;
            } else {
                reply_err_return(SYNTAX_ERR);
            }
        }
    }

    resp->reply_list_ready();
    if (direction == DIRECTION::BACKWARD){
        ret = ssdb->zrevrangebylex(ctx, req[1], req[2], req[3], resp->resp, offset, limit);
    } else{
        ret = ssdb->zrangebylex(ctx, req[1], req[2], req[3], resp->resp, offset, limit);
    }

    if (ret < 0){
        resp->resp.clear();
        reply_err_return(ret);
    }

    return 0;
}

int proc_zrangebylex(Context &ctx, const Request &req, Response *resp){
    VcServer *serv = ctx.serv;
    return _zrangebylex(ctx, serv->db, req, resp, DIRECTION::FORWARD);
}

int proc_zremrangebylex(Context &ctx, const Request &req, Response *resp){
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(4);

    int64_t count = 0;

    int ret = serv->db->zremrangebylex(ctx, req[1], req[2], req[3], &count);

    if (ret < 0) {
        reply_err_return(ret);
    }

    resp->reply_int(0, count);

    return 0;
}

int proc_zrevrangebylex(Context &ctx, const Request &req, Response *resp){
    VcServer *serv = ctx.serv;
    return _zrangebylex(ctx, serv->db, req, resp, DIRECTION::BACKWARD);
}

int proc_zlexcount(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(4);
    VcServer *serv = ctx.serv;

    int64_t count = 0;

    int ret = serv->db->zlexcount(ctx, req[1], req[2], req[3], &count);

    if (ret < 0){
        reply_err_return(SYNTAX_ERR);
    }

    resp->reply_int(0, count);

    return 0;
}
