//
// Created by zts on 12/18/17.
//
#include "slash/include/slash_string.h"
#include "slash/include/env.h"

#include "ClientConn.h"
#include "RedisJob.h"


VcClientConn::VcClientConn(int fd, const std::string &ip_port, pink::ServerThread *thread, void *worker_specific_data)
        : RedisConn(fd, ip_port, thread) {

    server = static_cast<VcServer *>(worker_specific_data);
    ctx = new Context();
    ctx->serv = server;
}

VcClientConn::~VcClientConn() {
    delete ctx;
}

int VcClientConn::ReplyError(const std::string& msg, std::string* response) {
    response->append("-");
    response->append(msg);
    response->append("\r\n");
}

int VcClientConn::DealMessage(pink::RedisCmdArgsType& argv, std::string* response) {
    if (argv.empty()) {
        ReplyError("empty request", response);
        return -2;
    }

    if (!server->status) {
        ReplyError("server not serving ~", response);
        return -2;
    }

    int64_t start_us = slash::NowMicros();

    RedisJob request(argv);
    request.convert_req();

    if(log_level() >= Logger::LEVEL_DEBUG) {
        log_debug("[receive] req: %s", serialize_req(request.req).c_str());
    }

    Command *cmd = server->proc_map.getProc(slash::StringToLower(request.cmd));
    if (!cmd) {
        ReplyError("command not found", response);
        return -2;
    }

    int result = (*cmd->proc)(*ctx, request.req, &(request.response));

    if (request.response.redisResponse != nullptr) {
        response->append(request.response.redisResponse->toRedis());
        delete request.response.redisResponse;
        request.response.redisResponse = nullptr;
    } else {
        request.convert_resq();
        response->append(request.output->data(), request.output->size());
    }


    int64_t time_proc = slash::NowMicros() - start_us;
    log_debug("[result]  p:%d, req: %s, resp: %s,",
              time_proc,
              serialize_req(request.req).c_str(),
              serialize_req(request.response.resp).c_str());

    return 0;
}