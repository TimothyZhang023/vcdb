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
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/swapdb

        ${SLASH_PATH}/
        ${PINK_PATH}/
        ${JEMALLOC_PATH}/include
        ${ROCKSDB_PATH}/include
        ${ROCKSDB_PATH}
        ${ZLIB_PATH}
        ${BZIP2_PATH}
)


LINK_DIRECTORIES(
        ${SLASH_PATH}/slash/lib
        ${PINK_PATH}/pink/lib/
        ${JEMALLOC_PATH}/lib/
        ${ROCKSDB_PATH}/
        ${SNAPPY_PATH}/
        ${BZIP2_PATH}/
        ${ZLIB_PATH}/
)


SET(RDB_OBJS
        src/swapdb/redis/crc/crc64speed.c
        src/swapdb/redis/crc/crcspeed.c
        src/swapdb/redis/ziplist.c
        src/swapdb/redis/intset.c
        src/swapdb/redis/lzf_c.c
        src/swapdb/redis/lzf_d.c
        src/swapdb/redis/endianconv.c
        src/swapdb/redis/zmalloc.c
        src/swapdb/redis/redis_encoder.cpp
        src/swapdb/redis/rdb_decoder.cpp
        src/swapdb/redis/sha1.c
        )
ADD_LIBRARY(librdb STATIC ${RDB_OBJS})
set_target_properties(librdb PROPERTIES LINKER_LANGUAGE CXX C_STANDARD 11 OUTPUT_NAME "rdb")

set(SWAPDB_SOURCE_FILES
        src/swapdb/serv.cpp
        src/swapdb/proc.cpp
        src/swapdb/proc_hash.cpp
        src/swapdb/proc_kv.cpp
        src/swapdb/proc_list.cpp
        src/swapdb/proc_set.cpp
        src/swapdb/proc_zset.cpp

        src/swapdb/ssdb/iterator.cpp
        src/swapdb/ssdb/options.cpp
        src/swapdb/ssdb/ssdb_impl.cpp
        src/swapdb/ssdb/t_cursor.cpp
        src/swapdb/ssdb/t_eset.cpp
        src/swapdb/ssdb/t_hash.cpp
        src/swapdb/ssdb/t_keys.cpp
        src/swapdb/ssdb/t_kv.cpp
        src/swapdb/ssdb/t_list.cpp
        src/swapdb/ssdb/t_set.cpp
        src/swapdb/ssdb/ttl.cpp
        src/swapdb/ssdb/t_zset.cpp

        src/swapdb/util/config.cpp
        src/swapdb/util/bytes.cpp
        src/swapdb/util/log.cpp

        src/swapdb/net/resp.cpp

        src/swapdb/codec/decode.cpp
        src/swapdb/codec/encode.cpp
        src/swapdb/codec/internal_error.cpp
        src/swapdb/codec/util.cpp

        )


set(SOURCE_FILES ${SWAPDB_SOURCE_FILES}
        src/main.cpp
        src/RedisJob.cpp
        src/Application.cpp
        src/ClientConn.cpp
        src/AppArgs.cpp
)


add_executable(vcdb ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(vcdb pink slash jemalloc pthread rocksdb librdb snappy z bz2)
