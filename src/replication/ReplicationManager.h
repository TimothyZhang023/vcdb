//
// Created by zts on 1/9/18.
//

#ifndef VCDB_REPLICATIONMANAGER_H
#define VCDB_REPLICATIONMANAGER_H

#include "string"
#include "map"
#include "SyncRedisMaster.h"

class ReplicationManager {
    SyncRedisMaster *redisMaster = nullptr;
    ServerContext *ctx = nullptr;


public:
    SyncRedisMaster *AddMaster(const std::string &ip, int port);

    int Start();

    int Stop();

    explicit ReplicationManager(ServerContext *ctx);

    virtual ~ReplicationManager();
};


#endif //VCDB_REPLICATIONMANAGER_H
