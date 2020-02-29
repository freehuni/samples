#=========================================================
# 2019/9/12 mh.kang@hanwha.com
# Create CMakeLists.txt for cmake environment
#=========================================================

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET(PROJECT_NAME "hello")
PROJECT(${PROJECT_NAME})

ADD_DEFINITIONS( -D_POSIX -D_LARGEFILE64_SOURCE=1 -DSIZE_64BIT_SUPPORT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64)

INCLUDE_DIRECTORIES(/home/mhkang/workspace/samples/opensource/protobuf/hello/cpp)

ADD_EXECUTABLE(${PROJECT_NAME}
    ./main.cpp
    ./cpp/hello.pb.cc
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} protobuf)
