//
// Created by zts on 12/18/17.
//
#include "slash/include/slash_string.h"
#include "slash/include/env.h"

#include "ClientConn.h"
#include "RedisJob.h"
#include "common/ServerContext.hpp"


vcdb::VcClientConn::VcClientConn(int fd, const std::string &ip_port, pink::ServerThread *thread, void *worker_specific_data)
        : pink::RedisConn(fd, ip_port, thread) {

    server = static_cast<ServerContext *>(worker_specific_data);
    ctx = new ClientContext();
    ctx->db = server->db;
}

vcdb::VcClientConn::~VcClientConn() {
    delete ctx;
}

int vcdb::VcClientConn::ReplyError(const std::string &msg, std::string *response) {
    response->append("-");
    response->append(msg);
    response->append("\r\n");
}

int vcdb::VcClientConn::DealMessage(pink::RedisCmdArgsType &argv, std::string *response) {
    if (argv.empty()) {
        ReplyError("empty request", response);
        return -2;
    }

    if (!server->status) {
        ReplyError("server not serving ~", response);
        return -2;
    }

    int64_t start_us = slash::NowMicros();

    RedisJob request(argv, response);

    if (log_level() >= Logger::LEVEL_DEBUG) {
        log_debug("[receive] req: %s", serialize_req(request.req).c_str());
    }

    Command *cmd = server->procMap.getProc(slash::StringToLower(request.cmd));
    if (cmd == nullptr) {
        ReplyError("command not found", response);
        return -2;
    }

    int result = (*cmd->proc)(*ctx, request.req, &(request.response));

    //-------response---------debug--------
    if (response->empty()) {
        log_error("bug detected?");
        ReplyError("empty response got for " + serialize_req(request.req), response);
    }
    //-------response---------debug--------

    int64_t time_proc = slash::NowMicros() - start_us;

    if (log_level() >= Logger::LEVEL_DEBUG) {
        log_debug("[result] p:%d, req: %s, resp: %s", time_proc, serialize_req(request.req).c_str(),
                  hexcstr(*response));
    }

    return 0;
}