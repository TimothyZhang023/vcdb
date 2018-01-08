//
// Created by zts on 1/8/18.
//
#include <pink/include/server_thread.h>
#include <util/log.h>
#include <pink/include/redis_conn.h>
#include "SyncRedisMaster.h"


SyncRedisMaster::~SyncRedisMaster() {
    delete master_client;
}

SyncRedisMaster::SyncRedisMaster(ServerContext *ctx, const std::string &ip, int port) :
        ctx(ctx), master_ip(ip), master_port(port) {
    master_client = vcdb::NewRedisClient();
}

ServerContext *SyncRedisMaster::getCtx() const {
    return ctx;
}


void *SyncRedisMaster::ThreadMain() {
    Status s = Status::OK(), result;

    while (!should_stop()) {
        result = master_client->Connect(master_ip, master_port);

        log_info ("Connect <%s:%d> %s ", master_ip.c_str(), master_port, result.ToString().c_str());
        if (result.ok()) {
            std::string cmd;

            cmd = RestoreRequest(std::vector<std::string>{"SYNC"});
            master_client->Send(&cmd);

            auto size = master_client->BufferRead();
            log_info ("got :%d bytes", size);

            while (size > 0) {
                auto buf = master_client->ReadBytes(static_cast<unsigned int>(size));

                if (buf != nullptr) {
                    std::string sbuf(buf, static_cast<unsigned long>(size));
                    log_info ("sbuf :%s", hexcstr(sbuf));

                    size = master_client->BufferRead();
                    log_info ("got :%d bytes", size);
                }
            }


            sleep(2000);

        }
        sleep(2);
    }
    return nullptr;
}

