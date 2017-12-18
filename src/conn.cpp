//
// Created by zts on 12/18/17.
//
#include "slash/include/slash_string.h"
#include "slash/include/env.h"

#include "conn.h"
#include "redis_cvt.h"

#include <swapdb/serv.h>


static std::map<std::string, std::string> db;


VcClientConn::VcClientConn(int fd, const std::string &ip_port, pink::ServerThread *thread, void *worker_specific_data)
        : RedisConn(fd, ip_port, thread) {
    // Handle worker_specific_data ...
    server = static_cast<SSDBServer *>(worker_specific_data);
    ctx = new Context();
    ctx->serv = server;
}

VcClientConn::~VcClientConn() {
    delete ctx;
}

int VcClientConn::ReplyError(const std::string& msg) {
    ExpandWbufTo(static_cast<uint32_t>(msg.size()) + 3);

    memcpy(wbuf_ + wbuf_len_, "-", 1);
    wbuf_len_ += 1;

    memcpy(wbuf_ + wbuf_len_, msg.data(), static_cast<size_t>(msg.size()));
    wbuf_len_ += msg.size();

    memcpy(wbuf_ + wbuf_len_, "\r\n", 2);
    wbuf_len_ += 2;

    set_is_reply(true);
}

int VcClientConn::DealMessage() {
    if (argv_.empty()) {
        ReplyError("empty request");
        return -2;
    }

    int64_t start_us = slash::NowMicros();

    RedisJob request(argv_);

    if(log_level() >= Logger::LEVEL_DEBUG) {
        log_debug("[receive] req: %s", serialize_req(request.req).c_str());
    }

    std::string &cmdName = argv_[0];
    slash::StringToLower(cmdName);

    Command *cmd = server->proc_map.get_proc(Bytes(cmdName));
    if (!cmd) {
        ReplyError("command not found");
        return -2;
    }

    request.convert_req();
    int result = (*cmd->proc)(*ctx, request.req, &(request.response));
    request.convert_resq();

    ExpandWbufTo(static_cast<uint32_t>(request.output->size()));
    memcpy(wbuf_ + wbuf_len_, request.output->data(), static_cast<size_t>(request.output->size()));
    wbuf_len_ += request.output->size();



    int64_t time_proc = slash::NowMicros() - start_us;
    log_debug("[result]  p:%d, req: %s, resp: %s,",
              time_proc,
              serialize_req(request.req).c_str(),
              serialize_req(request.response.resp).c_str());


    set_is_reply(true);
    return 0;
}





