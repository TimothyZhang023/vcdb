//
// Created by zts on 1/8/18.
//

#ifndef VCDB_SYNCREDISMASTER_H
#define VCDB_SYNCREDISMASTER_H

#include <pink/include/pink_conn.h>
#include <pink/include/pink_cli.h>
#include <ServerContext.hpp>
#include <RedisClient.h>


class MasterConnFactory;


class SyncRedisMaster : public pink::Thread {

public:
    virtual ~SyncRedisMaster();

    SyncRedisMaster(ServerContext *ctx, const std::string &ip, int port);

    ServerContext *getCtx() const;

private:

    ServerContext *ctx = nullptr;

    std::string master_ip = "127.0.0.1";
    int master_port = 6380;
    vcdb::RedisClient *master_client = nullptr;

    virtual void* ThreadMain();

};


#endif //VCDB_SYNCREDISMASTER_H
