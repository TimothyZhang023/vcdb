//
// Created by zts on 17-12-26.
//

#include "AppArgs.h"
#include <util/file.h>
#include <swapdb/util/strings.h>
#include <swapdb/util/log.h>


int AppArgs::readPid() {
    if (pidFile.empty()) {
        return -1;
    }
    std::string s;
    file_get_contents(pidFile, &s);
    if (s.empty()) {
        return -1;
    }
    return str_to_int(s);
}

void AppArgs::writePid() {
    if (!isDaemon) {
        return;
    }
    if (pidFile.empty()) {
        return;
    }
    int pid = (int) getpid();
    std::string s = str(pid);
    int ret = file_put_contents(pidFile, s);
    if (ret == -1) {
        log_error("Failed to write pidfile '%s'(%s)", pidFile.c_str(), strerror(errno));
        exit(1);
    }
}


void AppArgs::checkPidFile() {
    if (!isDaemon) {
        return;
    }
    if (pidFile.size()) {
        if (access(pidFile.c_str(), F_OK) == 0) {
            fprintf(stderr, "Fatal error!\nPidfile %s already exists!\n"
                            "Kill the running process before you run this command,\n"
                            "or use '-s restart' option to restart the server.\n",
                    pidFile.c_str());
            exit(1);
        }
    }
}

void AppArgs::removePidFile() {
    if (!isDaemon) {
        return;
    }
    if (pidFile.size()) {
        remove(pidFile.c_str());
    }
}

void AppArgs::killByPidFile() {
    int pid = readPid();
    if (pid == -1) {
        fprintf(stderr, "could not read pidfile: %s(%s)\n", pidFile.c_str(), strerror(errno));
        exit(1);
    }
    if (kill(pid, 0) == -1 && errno == ESRCH) {
        fprintf(stderr, "process: %d not running\n", pid);
        removePidFile();
        return;
    }
    int ret = kill(pid, SIGTERM);
    if (ret == -1) {
        fprintf(stderr, "could not kill process: %d(%s)\n", pid, strerror(errno));
        exit(1);
    }

    while (file_exists(pidFile)) {
        usleep(100 * 1000);
    }
}

