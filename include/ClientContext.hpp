//
// Created by zts on 12/18/17.
//

#ifndef VCDB_CONTEXT_H
#define VCDB_CONTEXT_H


class SSDBImpl;
class ReplicationManager;


class ClientContext {
public:
    SSDBImpl *db = nullptr;
    ReplicationManager *replicationManager = nullptr;
};


#endif //VCDB_CONTEXT_H
