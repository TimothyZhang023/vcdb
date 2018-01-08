cmake_minimum_required(VERSION 2.8)
project(vcdb)

set(CMAKE_CXX_STANDARD 11)

SET(SLASH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/slash)
SET(PINK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/pink)
SET(JEMALLOC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/jemalloc)
SET(ROCKSDB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/rocksdb)
SET(SNAPPY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/snappy)
SET(ZLIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/zlib)
SET(BZIP2_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/bzip2)
SET(CRCSPEED_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/crcspeed)


ADD_DEFINITIONS(-D__XDEBUG_H_)


# Set a default build type if none was specified
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Release" "Debug" "MinSizeRel" "RelWithDebInfo")
endif ()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

SET(LIBRARY_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/build/lib")
SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/build")


#execute_process(COMMAND
#        "./build-deps.sh"
#        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
#        )


INCLUDE_DIRECTORIES(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src

        ${SLASH_PATH}/
        ${PINK_PATH}/
        ${JEMALLOC_PATH}/include
        ${ROCKSDB_PATH}/include
        ${ROCKSDB_PATH}
        ${ZLIB_PATH}
        ${BZIP2_PATH}
        ${CRCSPEED_PATH}
)


LINK_DIRECTORIES(
        ${SLASH_PATH}/slash/lib
        ${PINK_PATH}/pink/lib/
        ${JEMALLOC_PATH}/lib/
        ${ROCKSDB_PATH}/
        ${SNAPPY_PATH}/
        ${BZIP2_PATH}/
        ${ZLIB_PATH}/
        ${CRCSPEED_PATH}/
)


SET(RDB_OBJS
        src/redis/ziplist.c
        src/redis/intset.c
        src/redis/lzf_c.c
        src/redis/lzf_d.c
        src/redis/endianconv.c
        src/redis/zmalloc.c
        src/redis/sha1.c
        )
ADD_LIBRARY(librdb STATIC ${RDB_OBJS})
set_target_properties(librdb PROPERTIES LINKER_LANGUAGE CXX C_STANDARD 11 OUTPUT_NAME "rdb")


SET(CRCSPEED_SRC
        ${CRCSPEED_PATH}/crc64speed.c
        ${CRCSPEED_PATH}/crc16speed.c
        ${CRCSPEED_PATH}/crcspeed.c
        )
ADD_LIBRARY(libcrcspeed STATIC ${CRCSPEED_SRC})
set_target_properties(libcrcspeed PROPERTIES LINKER_LANGUAGE CXX C_STANDARD 11 OUTPUT_NAME "crcspeed")


set(SWAPDB_SOURCE_FILES
        src/proc/proc_server.cpp
        src/proc/proc_hash.cpp
        src/proc/proc_kv.cpp
        src/proc/proc_list.cpp
        src/proc/proc_set.cpp
        src/proc/proc_zset.cpp
        src/proc/proc_common.h

        src/storage/iterator.cpp
        src/storage/options.cpp
        src/storage/ssdb_impl.cpp
        src/storage/t_cursor.cpp
        src/storage/t_eset.cpp
        src/storage/t_hash.cpp
        src/storage/t_keys.cpp
        src/storage/t_kv.cpp
        src/storage/t_list.cpp
        src/storage/t_set.cpp
        src/storage/ttl.cpp
        src/storage/t_zset.cpp
        src/storage/VcRocksEventListener.h
        src/storage/t_scan.h

        )

set(CODEC_SOURCE_FILES
        src/codec/kv/decode.cpp
        src/codec/kv/encode.cpp

        src/codec/internal_error.cpp

        src/codec/rdb/RdbEncoder.cpp
        src/codec/rdb/RdbDecoder.cpp

        src/codec/util.cpp
        )

set(UTIL_SOURCE_FILES
        src/common/Response.cpp

        src/util/config.cpp
        src/util/bytes.cpp
        src/util/log.cpp
        )

set(HEADERS_FILES
        include/Constant.h
        include/Binlog.h
        include/ClientContext.hpp
        include/ServerContext.hpp
        include/Commands.h
        include/Request.h
        include/Response.h
        include/ProcMap.h
        include/ProcDefine.h

        )


set(SOURCE_FILES ${HEADERS_FILES} ${CODEC_SOURCE_FILES} ${SWAPDB_SOURCE_FILES} ${UTIL_SOURCE_FILES}
        src/main.cpp
        src/RedisJob.cpp
        src/Application.cpp
        src/ClientConn.cpp
        src/AppArgs.cpp

        src/common/ProcMap.cpp

        src/Binlog.cpp
        src/replication/SyncRedisMaster.cpp src/replication/SyncRedisMaster.h
        src/RedisClient.cpp src/RedisClient.h)


add_executable(vcdb ${SOURCE_FILES} )

TARGET_LINK_LIBRARIES(vcdb pink slash jemalloc pthread rocksdb librdb snappy z bz2 libcrcspeed)
