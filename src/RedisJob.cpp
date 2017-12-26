//
// Created by zts on 12/18/17.
//

#include "RedisJob.h"

#define INIT_BUFFER_SIZE  1024
#define BEST_BUFFER_SIZE  (8 * 1024)



RedisJob::RedisJob(const std::vector<std::string> &recv_string) : recv_string(recv_string) {
    for (const auto &t : recv_string) {
        req.emplace_back(t);
    }

    cmd = recv_string.at(0);

    output = new Buffer(INIT_BUFFER_SIZE);

}

RedisJob::~RedisJob() {

    delete output;
}

int RedisJob::convert_req() {
    if(!inited){
        inited = true;

        RedisCommand_raw *def = &cmds_raw[0];
        while(def->redis_cmd != nullptr){
            RedisRequestDesc desc;
            desc.strategy = def->strategy;
            desc.redis_cmd = def->redis_cmd;
            desc.ssdb_cmd = def->ssdb_cmd;
            desc.reply_type = def->reply_type;
            cmd_table[desc.redis_cmd] = desc;
            def += 1;
        }
    }

    this->req_desc = nullptr;

    const RedisRequestConvertTable::iterator &it = cmd_table.find(cmd);
    if(it == cmd_table.end()){
        return -1;
    }

    this->req_desc = &(it->second);
    cmd = this->req_desc->ssdb_cmd;

    return 0;
}

int RedisJob::convert_resq() {
    const std::vector<std::string> &resp = response.resp;

    if(resp.empty()){
        return 0;
    }
    if(resp[0] != "ok"){
        if(resp[0] == "error" || resp[0] == "fail" || resp[0] == "client_error"){
            if(resp.size() >= 2){
                output->append("-");
                output->append(resp[1]);
            } else {
                output->append("-ERR ");
            }
            output->append("\r\n");
        }else if(resp[0] == "not_found"){
            output->append("$-1\r\n");
        }else if(resp[0] == "noauth"){
            output->append("-NOAUTH ");
            if(resp.size() >= 2){
                output->append(resp[1]);
            }
            output->append("\r\n");
        }else{
            output->append("-ERR server error\r\n");
        }
        return 0;
    }

    // not supported command
    if(req_desc == nullptr){
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "*%d\r\n", (int)resp.size() - 1);
            output->append(buf);
        }
        for(int i=1; i<resp.size(); i++){
            const std::string &val = resp[i];
            char buf[32];
            snprintf(buf, sizeof(buf), "$%d\r\n", (int)val.size());
            output->append(buf);
            output->append(val.data(), val.size());
            output->append("\r\n");
        }
        return 0;
    }
    if(req_desc->reply_type == REPLY_INFO){

        std::string tmp;
        for(int i=1; i<resp.size(); i++){
            const std::string &val = resp[i];
            tmp.append(val);
            tmp.append("\r\n");
        }

        char buf[32];
        snprintf(buf, sizeof(buf), "$%d\r\n", (int)tmp.size());
        output->append(buf);
        output->append(tmp.data(), tmp.size());
        output->append("\r\n");

        return 0;
    }
    if(req_desc->strategy == STRATEGY_PING){
        output->append("+PONG\r\n");
        return 0;
    }
    if(req_desc->reply_type == REPLY_OK_STATUS){
        output->append("+OK\r\n");
        return 0;
    }
    if(req_desc->reply_type == REPLY_CUSTOM_STATUS){
        if(resp.size() >= 2) {
            output->append("+");
            output->append(resp[1]);
            output->append("\r\n");
        } else {
            output->append("+OK\r\n");
        }

        return 0;
    }
    if(req_desc->reply_type == REPLY_SET_STATUS){
        if(resp.size() >= 2) {
            if (resp[1] == "1") {
                output->append("+OK\r\n");
            } else if (resp[1] == "0") {
                output->append("$-1\r\n");
            } else {
                output->append("-ERR server error, check REPLY_SET_STATUS\r\n");
            }
        } else {
            output->append("+OK\r\n");
        }

        return 0;
    }
    if(req_desc->reply_type == REPLY_BULK){
        if(resp.size() >= 2){
            const std::string &val = resp[1];
            char buf[32];
            snprintf(buf, sizeof(buf), "$%d\r\n", (int)val.size());
            output->append(buf);
            output->append(val.data(), val.size());
            output->append("\r\n");
        }else{
            output->append("$0\r\n");
        }
        return 0;
    }
    if(req_desc->reply_type == REPLY_INT){
        if(resp.size() >= 2){
            const std::string &val = resp[1];
            output->append(":");
            output->append(val.data(), val.size());
            output->append("\r\n");
        }else{
            output->append("$0\r\n");
        }
        return 0;
    }
    if(req_desc->strategy == STRATEGY_MGET || req_desc->strategy == STRATEGY_HMGET){
        if(resp.size() % 2 != 1){
            output->append("*0");
            //log_error("bad response for multi_(h)get");
            return 0;
        }
        char buf[32];
        std::vector<std::string>::const_iterator req_it, resp_it;
        if(req_desc->strategy == STRATEGY_MGET){
            req_it = recv_string.begin() + 1;
            snprintf(buf, sizeof(buf), "*%d\r\n", (int)recv_string.size() - 1);
        }else{
            req_it = recv_string.begin() + 2;
            snprintf(buf, sizeof(buf), "*%d\r\n", (int)recv_string.size() - 2);
        }
        output->append(buf);

        resp_it = resp.begin() + 1;

        while(req_it != recv_string.end()){
            const std::string &req_key = *req_it;
            req_it ++;
            if(resp_it == resp.end()){
                output->append("$-1\r\n");
                continue;
            }
            const std::string &resp_key = *resp_it;
            //log_debug("%s %s", req_key.c_str(), resp_key.c_str());
            if(req_key != resp_key){
                output->append("$-1\r\n");
                // loop until we find value to the requested key
                continue;
            }

            const std::string &val = *(resp_it + 1);
            char buf[32];
            snprintf(buf, sizeof(buf), "$%d\r\n", (int)val.size());
            output->append(buf);
            output->append(val.data(), val.size());
            output->append("\r\n");

            resp_it += 2;
        }

        return 0;
    }

    if(req_desc->reply_type == REPLY_SPOP_SRANDMEMBER){

        if (recv_string.size() == 2){

            if (resp.size() == 1) {
                output->append("$-1\r\n");
            } else {
                const std::string &val = resp[1];
                char buf[32];
                snprintf(buf, sizeof(buf), "$%d\r\n", (int)val.size());
                output->append(buf);
                output->append(val.data(), val.size());
                output->append("\r\n");
            }
        } else {
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "*%d\r\n", ((int)resp.size() - 1));
                output->append(buf);
            }
            for(int i=1; i<resp.size(); i++){
                const std::string &val = resp[i];
                char buf[32];
                snprintf(buf, sizeof(buf), "$%d\r\n", (int)val.size());
                output->append(buf);
                output->append(val.data(), val.size());
                output->append("\r\n");
            }
        }

        return 0;
    }

    if(req_desc->reply_type == REPLY_SCAN){

        do {
            if (resp.size() < 2) {
                output->append("-ERR server error when scan\r\n");
                break;
            }

            output->append("*2\r\n");

            const std::string &cursor = resp[1];
            char buf[32];
            snprintf(buf, sizeof(buf), "$%d\r\n", (int)cursor.size());
            output->append(buf);
            output->append(cursor.data(), cursor.size());
            output->append("\r\n");

            size_t count = resp.size() - 2;

            snprintf(buf, sizeof(buf), "*%d\r\n", (int)count);
            output->append(buf);

            for(int i=2; i<resp.size(); i++){
                const std::string &val = resp[i];
                char buf[32];
                snprintf(buf, sizeof(buf), "$%d\r\n", (int)val.size());
                output->append(buf);
                output->append(val.data(), val.size());
                output->append("\r\n");
            }

        } while (0);

        return 0;
    }

    if(req_desc->reply_type == REPLY_MULTI_BULK){
        bool withscores = true;
        if(req_desc->strategy == STRATEGY_ZRANGE || req_desc->strategy == STRATEGY_ZREVRANGE){
            if(recv_string.size() < 5 || recv_string[4] != "withscores"){
                withscores = false;
            }
        }
        if(req_desc->strategy == STRATEGY_ZRANGEBYSCORE || req_desc->strategy == STRATEGY_ZREVRANGEBYSCORE){
            if(recv_string[recv_string.size() - 1] != "withscores"){
                withscores = false;
            }
        }
        {
            char buf[32];
            if(withscores){
                snprintf(buf, sizeof(buf), "*%d\r\n", (int)resp.size() - 1);
            }else{
                snprintf(buf, sizeof(buf), "*%d\r\n", ((int)resp.size() - 1)/2);
            }
            output->append(buf);
        }
        for(int i=1; i<resp.size(); i++){
            const std::string &val = resp[i];
            char buf[32];
            snprintf(buf, sizeof(buf), "$%d\r\n", (int)val.size());
            output->append(buf);
            output->append(val.data(), val.size());
            output->append("\r\n");
            if(!withscores){
                i += 1;
            }
        }
        return 0;
    }

    output->append("-ERR server error\r\n");
    return 0;
}
