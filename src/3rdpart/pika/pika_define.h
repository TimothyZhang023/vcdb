//
// Created by zts on 2/27/18.
//

#ifndef VCDB_PIKA_DEFINE_H
#define VCDB_PIKA_DEFINE_H


/*
 * The size of Binlogfile
 */
//static uint64_t kBinlogSize = 128;
//static const uint64_t kBinlogSize = 1024 * 1024 * 100;

enum RecordType {
    kZeroType = 0,
    kFullType = 1,
    kFirstType = 2,
    kMiddleType = 3,
    kLastType = 4,
    kEof = 5,
    kBadRecord = 6,
    kOldRecord = 7
};

/*
 * the block size that we read and write from write2file
 * the default size is 64KB
 */
static const size_t kBlockSize = 64 * 1024;

/*
 * Header is Type(1 byte), length (3 bytes), time (4 bytes)
 */
static const size_t kHeaderSize = 1 + 3 + 4;

/*
 * the size of memory when we use memory mode
 * the default memory size is 2GB
 */
const int64_t kPoolSize = 1073741824;

const std::string kBinlogPrefix = "write2file";
const size_t kBinlogPrefixLen = 10;

const std::string kManifest = "manifest";


#endif //VCDB_PIKA_DEFINE_H
