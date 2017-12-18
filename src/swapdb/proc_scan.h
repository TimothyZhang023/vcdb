//
// Created by zts on 12/18/17.
//

#ifndef VCDB_PROC_SCAN_H
#define VCDB_PROC_SCAN_H

#include <vector>
#include <string>
#include "util/bytes.h"
#include "codec/error.h"

class ScanParams {
public:
    std::string pattern = "*";
    uint64_t limit = 10;
};


inline int prepareForScanParams(std::vector<Bytes> req, int startIndex, ScanParams &scanParams) {

    std::vector<Bytes>::const_iterator it = req.begin() + startIndex;
    for (; it != req.end(); it += 2) {
        std::string key = (*it).String();
        strtolower(&key);

        if (key == "match") {
            scanParams.pattern = (*(it + 1)).String();
        } else if (key == "count") {
            scanParams.limit = (*(it + 1)).Uint64();
            if (errno == EINVAL) {
                return INVALID_INT;
            }
        } else {
            return SYNTAX_ERR;
        }
    }

    return 0;
}



#endif //VCDB_PROC_SCAN_H
