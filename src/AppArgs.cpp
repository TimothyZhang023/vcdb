//
// Created by zts on 17-12-26.
//

#include "AppArgs.h"
#include <util/file.h>
#include <swapdb/util/strings.h>
#include <swapdb/util/log.h>


int AppArgs::ReadPid() {
    if (pid_file.empty()) {
        return -1;
    }
    std::string s;
    file_get_contents(pid_file, &s);
    if (s.empty()) {
        return -1;
    }
    return str_to_int(s);
}

void AppArgs::WritePid() {
    if (!is_daemon) {
        return;
    }
    if (pid_file.empty()) {
        return;
    }
    int pid = (int) getpid();
    std::string s = str(pid);
    int ret = file_put_contents(pid_file, s);
    if (ret == -1) {
        log_error("Failed to write pidfile '%s'(%s)", pid_file.c_str(), strerror(errno));
        exit(1);
    }
}


void AppArgs::CheckPidFile() {
    if (!is_daemon) {
        return;
    }
    if (pid_file.size()) {
        if (access(pid_file.c_str(), F_OK) == 0) {
            fprintf(stderr, "Fatal error!\nPidfile %s already exists!\n"
                            "Kill the running process before you run this command,\n"
                            "or use '-s restart' option to restart the server.\n",
                    pid_file.c_str());
            exit(1);
        }
    }
}

void AppArgs::RemovePidFile() {
    if (!is_daemon) {
        return;
    }
    if (pid_file.size()) {
        remove(pid_file.c_str());
    }
}

void AppArgs::KillByPidFile() {
    int pid = ReadPid();
    if (pid == -1) {
        fprintf(stderr, "could not read pidfile: %s(%s)\n", pid_file.c_str(), strerror(errno));
        exit(1);
    }
    if (kill(pid, 0) == -1 && errno == ESRCH) {
        fprintf(stderr, "process: %d not running\n", pid);
        RemovePidFile();
        return;
    }
    int ret = kill(pid, SIGTERM);
    if (ret == -1) {
        fprintf(stderr, "could not kill process: %d(%s)\n", pid, strerror(errno));
        exit(1);
    }

    while (file_exists(pid_file)) {
        usleep(100 * 1000);
    }
}

