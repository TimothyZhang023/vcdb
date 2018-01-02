/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include <cassert>
#include "proc.h"


ProcMap::ProcMap() {
}

ProcMap::~ProcMap() {
    for (auto &it : proc_map) {
        delete it.second;
    }
    proc_map.clear();
}

void ProcMap::regProc(Command *c) {
    Command *cmd = this->getProc(c->name);
    if (cmd == nullptr) {
        proc_map[c->name] = c;
    } else {
        static_assert(true, "dup Command");
    }
}

Command *ProcMap::getProc(const std::string &str) {
    auto it = proc_map.find(str);
    if (it != proc_map.end()) {
        return it->second;
    }
    return nullptr;
}
