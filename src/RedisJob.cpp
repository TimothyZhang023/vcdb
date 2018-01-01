//
// Created by zts on 12/18/17.
//

#include "RedisJob.h"

namespace vcdb {

    RedisJob::RedisJob(const std::vector<std::string> &recv_string, std::string *res) :
            recv_string(recv_string), response(Response(res)) {
        for (const auto &t : recv_string) {
            req.emplace_back(t);
        }
        cmd = recv_string.at(0);
    }

    RedisJob::~RedisJob() {
    }

    int RedisJob::convert_req() {
        if (!inited) {
            inited = true;

            RedisCommand_raw *def = &cmds_raw[0];
            while (def->redis_cmd != nullptr) {
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
        if (it == cmd_table.end()) {
            return -1;
        }

        this->req_desc = &(it->second);
        cmd = this->req_desc->ssdb_cmd;

        return 0;
    }
}
