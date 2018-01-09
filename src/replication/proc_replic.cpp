//
// Created by zts on 1/9/18.
//
#include <ClientContext.hpp>
#include <Request.h>
#include <Response.h>
#include <proc/proc_common.h>
#include <replication/ReplicationManager.h>

int proc_slaveof(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);
    std::string ip = req[1].String();
    std::string s_port = req[2].String();

    strtoupper(&ip);
    strtoupper(&s_port);

    if (ip == "NO" && s_port == "ONE") {
        ctx.replicationManager->Stop();
    } else {
        int port = req[2].Int();
        ctx.replicationManager->AddMaster(ip, port);
        ctx.replicationManager->Start();
    }

    resp->addReplyStatusOK();
    return 0;
}
