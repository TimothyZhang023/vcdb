/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "proc.h"


ProcMap::ProcMap() {
}

ProcMap::~ProcMap() {
    for (auto &it : proc_map) {
        delete it.second;
    }
    proc_map.clear();
}

void ProcMap::regProc(const std::string &c, const char *sflags, proc_t proc) {
    Command *cmd = this->getProc(c);
    if (cmd == nullptr) {
        cmd = new Command();
        cmd->name = c;
        proc_map[cmd->name] = cmd;
    }
    cmd->proc = proc;
    cmd->flags = 0;
    for (const char *p = sflags; *p != '\0'; p++) {
        switch (*p) {
            case 'r':
                cmd->flags |= Command::FLAG_READ;
                break;
            case 'w': // w 必须和 t 同时出现, 因为某些写操作依赖单线程
                cmd->flags |= Command::FLAG_WRITE;
                cmd->flags |= Command::FLAG_THREAD;
                break;
            case 'b':
                cmd->flags |= Command::FLAG_BACKEND;
                break;
            case 't':
                cmd->flags |= Command::FLAG_THREAD;
                break;
        }
    }
}

Command *ProcMap::getProc(const std::string &str) {
    auto it = proc_map.find(str);
    if (it != proc_map.end()) {
        return it->second;
    }
    return nullptr;
}
