//
// Created by zts on 17-12-26.
//

#ifndef VCDB_APPARGS_H
#define VCDB_APPARGS_H

#include "string"

class AppArgs {
public:
    AppArgs() {
        isDaemon = false;
        action = "start";
    }

    int readPid();

    void writePid();

    void checkPidFile();
    void removePidFile();
    void killByPidFile();


public:
    bool isDaemon;
    int port = 6379;
    std::string ip = "0.0.0.0";
    std::string pidFile;
    std::string confFile;
    std::string workDir;
    std::string action;

};


#endif //VCDB_APPARGS_H
