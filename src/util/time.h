//
// Created by zts on 1/4/18.
//

#ifndef VCDB_TIME_H
#define VCDB_TIME_H




static inline int64_t time_ms() {
    struct timeval now;
    gettimeofday(&now, nullptr);
    return (int64_t) now.tv_sec * 1000 + (int64_t) now.tv_usec / 1000;
}




#endif //VCDB_TIME_H
