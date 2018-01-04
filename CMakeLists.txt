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
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/swapdb

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
        src/swapdb/serv.cpp
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


set(SOURCE_FILES ${CODEC_SOURCE_FILES} ${SWAPDB_SOURCE_FILES} ${UTIL_SOURCE_FILES}
        src/main.cpp
        src/RedisJob.cpp
        src/Application.cpp
        src/ClientConn.cpp
        src/AppArgs.cpp

        src/Constant.h

        src/common/ProcMap.cpp
        src/common/ClientContext.hpp
        src/common/ServerContext.hpp
        src/common/Commands.h
        src/common/Request.h

        )


add_executable(vcdb ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(vcdb pink slash jemalloc pthread rocksdb librdb snappy z bz2 libcrcspeed)
