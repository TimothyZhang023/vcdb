//
// Created by zts on 17-12-26.
//

#ifndef VCDB_APPARGS_H
#define VCDB_APPARGS_H

#include "string"

class AppArgs {
public:
    AppArgs() {
        is_daemon = false;
        action = "start";
    }

    int ReadPid();

    void WritePid();

    void CheckPidFile();

    void RemovePidFile();

    void KillByPidFile();

public:
    bool is_daemon;
    int port = 6379;
    std::string ip = "0.0.0.0";
    std::string pid_file;
    std::string conf_file;
    std::string work_dir;
    std::string action;

};


#endif //VCDB_APPARGS_H
