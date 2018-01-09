//
// Created by zts on 1/9/18.
//

#include "ReplicationManager.h"

SyncRedisMaster *ReplicationManager::AddMaster(const std::string &ip, int port) {
    std::string key = ip + ":" + str(port);

    if (redisMaster != nullptr) {
        redisMaster->StopThread();
        delete redisMaster;
        redisMaster = nullptr;
    }

    redisMaster = new SyncRedisMaster(ctx, ip, port);

    return redisMaster;
}


ReplicationManager::~ReplicationManager() {
    if (redisMaster != nullptr) {
        redisMaster->StopThread();
        delete redisMaster;
        redisMaster = nullptr;
    }
}

ReplicationManager::ReplicationManager(ServerContext *ctx) : ctx(ctx) {
}

int ReplicationManager::Start() {
    if (redisMaster != nullptr) {
        return redisMaster->StartThread();
    }
    return 0;
}

int ReplicationManager::Stop() {
    if (redisMaster != nullptr) {
        redisMaster->StopThread();
        delete redisMaster;
        redisMaster = nullptr;
    }
    return 0;
}
