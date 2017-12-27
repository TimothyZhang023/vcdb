//
// Created by zts on 12/18/17.
//

#ifndef VCDB_PROC_H
#define VCDB_PROC_H

#include <string>
#include <vector>
#include <unordered_map>

class Command;
class Context;
class Bytes;
class Response;

typedef std::vector<Bytes> Request;

typedef std::unordered_map<std::string, Command *> proc_map_t;
typedef int (*proc_t)(Context &ctx, const Request &req, Response *resp);


#define REG_PROC(c, f)  procMap.regProc(#c, f, proc_##c)


class ProcMap {
private:
    proc_map_t proc_map;

public:
    ProcMap();

    ~ProcMap();

    void regProc(const std::string &cmd, const char *sflags, proc_t proc);

    Command *getProc(const std::string &str);

    proc_map_t::iterator begin() {
        return proc_map.begin();
    }

    proc_map_t::iterator end() {
        return proc_map.end();
    }
};



struct Command {
    static const int FLAG_READ = (1 << 0);
    static const int FLAG_WRITE = (1 << 1);
    static const int FLAG_BACKEND = (1 << 2);
    static const int FLAG_THREAD = (1 << 3);

    std::string name;
    int flags;
    proc_t proc;
    uint64_t calls;
    double time_wait;
    double time_proc;

    Command() {
        flags = 0;
        proc = nullptr;
        calls = 0;
        time_wait = 0;
        time_proc = 0;
    }
};



#endif //VCDB_PROC_H
