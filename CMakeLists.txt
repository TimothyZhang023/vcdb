cmake_minimum_required(VERSION 2.8)
project(vcdb)

set(CMAKE_CXX_STANDARD 11)


SET(SLASH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/slash)
SET(PINK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/pink)
SET(JEMALLOC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/jemalloc)


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


execute_process(COMMAND
        "./build-deps.sh"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        )


INCLUDE_DIRECTORIES(
        ${SLASH_PATH}/
        ${PINK_PATH}/
        ${JEMALLOC_PATH}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)


LINK_DIRECTORIES(
        ${SLASH_PATH}/slash/lib
        ${PINK_PATH}/pink/lib/
        ${JEMALLOC_PATH}/lib/
)




set(PINK_SOURCE_FILES
        deps/pink/pink/src/bg_thread.cc
        deps/pink/pink/src/build_version.cc.in
        deps/pink/pink/src/dispatch_thread.cc
        deps/pink/pink/src/holy_thread.cc
        deps/pink/pink/src/http_conn.cc
        #deps/pink/pink/src/pb_cli.cc
        #deps/pink/pink/src/pb_conn.cc
        deps/pink/pink/src/period_thread.cc
        deps/pink/pink/src/pink_cli.cc
        deps/pink/pink/src/pink_conn.cc
        deps/pink/pink/src/pink_epoll.cc
        deps/pink/pink/src/pink_item.cc
        deps/pink/pink/src/pink_thread.cc
        deps/pink/pink/src/pink_util.cc
        deps/pink/pink/src/redis_cli.cc
        deps/pink/pink/src/redis_conn.cc
        deps/pink/pink/src/server_socket.cc
        deps/pink/pink/src/server_thread.cc
        deps/pink/pink/src/simple_http_conn.cc
        deps/pink/pink/src/worker_thread.cc
        )

set(SLASH_SOURCE_FILES
        deps/slash/slash/src/base_conf.cc
        deps/slash/slash/src/build_version.cc.in
        deps/slash/slash/src/cond_lock.cc
        deps/slash/slash/src/env.cc
        deps/slash/slash/src/posix.cc
        deps/slash/slash/src/rsync.cc
        deps/slash/slash/src/slash_binlog_impl.cc
        deps/slash/slash/src/slash_coding.cc
        deps/slash/slash/src/slash_hash.cc
        deps/slash/slash/src/slash_mutex.cc
        deps/slash/slash/src/slash_status.cc
        deps/slash/slash/src/slash_string.cc
        deps/slash/slash/src/slash_testharness.cc
        deps/slash/slash/src/testutil.cc
        )


set(SOURCE_FILES src/main.cpp src/app.cpp src/conn.cpp src/conn.h)


add_executable(vcdb ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(vcdb pink slash jemalloc pthread)