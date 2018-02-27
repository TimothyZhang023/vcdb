//
// Created by zts on 2/27/18.
//

#include "BinlogManager.h"

bool BinlogManager::GetBinlogFiles(std::map<uint32_t, std::string> &binlogs) {
    std::vector<std::string> children;
    int ret = slash::GetChildren(log_path, children);
    if (ret != 0) {
        log_warn("Get all files in log path failed! error: %d", ret);
        return false;
    }

    int64_t index = 0;
    std::string sindex;
    std::vector<std::string>::iterator it;
    for (it = children.begin(); it != children.end(); ++it) {
        if ((*it).compare(0, kBinlogPrefixLen, kBinlogPrefix) != 0) {
            continue;
        }
        sindex = (*it).substr(kBinlogPrefixLen);
        if (slash::string2l(sindex.c_str(), sindex.size(), &index) == 1) {
            binlogs.insert(std::pair<uint32_t, std::string>(static_cast<uint32_t>(index), *it));
        }
    }
    return true;
}

bool BinlogManager::PurgeFiles(uint32_t to, bool manual, bool force) {
    std::map<uint32_t, std::string> binlogs;
    if (!GetBinlogFiles(binlogs)) {
        log_warn("Could not get binlog files!");
        return false;
    }

    int delete_num = 0;
    struct stat file_stat;
    int remain_expire_num = binlogs.size() - expire_logs_nums;
    std::map<uint32_t, std::string>::iterator it;
    for (it = binlogs.begin(); it != binlogs.end(); ++it) {
        if ((manual && it->first <= to) ||           // Argument bound
            remain_expire_num > 0 ||                 // Expire num trigger
            (stat(((log_path + it->second)).c_str(), &file_stat) == 0 &&
             file_stat.st_mtime < time(NULL) - expire_logs_days * 24 * 3600)) // Expire time trigger
        {
            // We check this every time to avoid lock when we do file deletion
            if (!CouldPurge(it->first) && !force) {
                log_warn("Could not purge , since it is already be used", (it->first));
                return false;
            }

            // Do delete
            slash::Status s = slash::DeleteFile(log_path + it->second);
            if (s.ok()) {
                ++delete_num;
                --remain_expire_num;
                if (remain_expire_num <= 2) {
                    break;
                }
            } else {
                log_warn("Purge log file : %d  failed! error: %s", it->second, s.ToString().c_str());
            }
        } else {
            // Break when face the first one not satisfied
            // Since the binlogs is order by the file index
            break;
        }
    }
    if (delete_num) {
        log_info("Success purge %d", delete_num);
    }

    return true;
}

bool BinlogManager::CouldPurge(uint32_t index) {
//    uint32_t pro_num;
//    uint64_t tmp;
//    logger_->GetProducerStatus(&pro_num, &tmp);
//
//    index += 10; //remain some more
//    if (index > pro_num) {
//        return false;
//    }

    //check slave

    return true;
}

BinlogManager::BinlogManager(Binlog *logger_, const string &log_path, int64_t expire_logs_nums,
                             int64_t expire_logs_days) : logger_(logger_), log_path(log_path),
                                                         expire_logs_nums(expire_logs_nums),
                                                         expire_logs_days(expire_logs_days) {}
