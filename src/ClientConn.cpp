//
// Created by zts on 12/18/17.
//
#include "slash/include/slash_string.h"
#include "slash/include/env.h"
#include "pika/pika_binlog.h"

#include "ClientConn.h"
#include "RedisJob.h"
#include "ServerContext.hpp"
#include "ClientContext.hpp"
#include "util/log.h"


vcdb::VcClientConn::VcClientConn(int fd, const std::string &ip_port, pink::ServerThread *thread,
                                 void *worker_specific_data)
        : pink::RedisConn(fd, ip_port, thread) {

    server = static_cast<ServerContext *>(worker_specific_data);
    binlog = server->binlog;
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
        ReplyError("EMPTY REQUEST", response);
        return -2;
    }

    int64_t start_us = slash::NowMicros();

    RedisJob request(argv, response);

    if (log_level() >= Logger::LEVEL_DEBUG) {
        log_debug("[receive] req: %s", SerializeRequest(request.req).c_str());
    }

    Command *cmd = server->procMap.getProc(slash::StringToLower(request.cmd));
    if (cmd == nullptr) {
        ReplyError("COMMAND NOT FOUND", response);
        return 0;
    }

    if (server->status != SERVER_RUNNING) {
        if (server->status == SERVER_LOADING) {
            if (!(cmd->flags & CMD_LOADING)) {
                ReplyError("LOADING", response);
                return 0;
            }
        }

        ReplyError("SERVER NOT RUNNING", response);
        return -2;
    }

    std::string key;
    if (argv.size() > 1) {
        key = argv[1];
    }

    if (cmd->flags & CMD_WRITE) {
        binlog->LockKey(key);
    }

    int result = (*cmd->proc)(*ctx, request.req, &(request.response));

    //-------response---------debug--------
    if (response->empty()) {
        log_error("bug detected?");
        ReplyError("EMPTY RESPONSE FOR " + SerializeRequest(request.req), response);
    }
    //-------response---------debug--------

    int64_t time_proc = slash::NowMicros() - start_us;

    if (log_level() >= Logger::LEVEL_DEBUG) {
        log_debug("[result] process:%d, req: %s, resp: %s",
                  time_proc, SerializeRequest(request.req).c_str(), hexcstr(*response));
    }

    if (cmd->flags & CMD_WRITE) {
        binlog->Lock();
        if (!(request.response.getStatus() & RESP_ERR)) {
            log_debug("write success %s", hexcstr(RestoreRequest(request.req)));
            binlog->Put(RestoreRequest(request.req));
        }
        binlog->UnlockKey(key);
        binlog->Unlock();
    }


    return 0;
}