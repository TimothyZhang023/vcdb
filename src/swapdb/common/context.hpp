//
// Created by zts on 12/18/17.
//

#ifndef VCDB_CONTEXT_H
#define VCDB_CONTEXT_H


//class NetworkServer;
class SSDBServer;


class Context {
public:
//    NetworkServer *net = nullptr;
    SSDBServer *serv = nullptr;
};


#endif //VCDB_CONTEXT_H
