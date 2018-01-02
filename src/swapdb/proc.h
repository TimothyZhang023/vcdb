//
// Created by zts on 12/18/17.
//

#ifndef VCDB_PROC_H
#define VCDB_PROC_H

#include <string>
#include <vector>
#include <unordered_map>
#include <Commands.h>


typedef std::unordered_map<std::string, Command *> proc_map_t;

class ProcMap {
private:
    proc_map_t proc_map;

public:
    ProcMap();

    ~ProcMap();

    void regProc(Command *c);

    Command *getProc(const std::string &str);

    proc_map_t::iterator begin() {
        return proc_map.begin();
    }

    proc_map_t::iterator end() {
        return proc_map.end();
    }
};


#endif //VCDB_PROC_H
