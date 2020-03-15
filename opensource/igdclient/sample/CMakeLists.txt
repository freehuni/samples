cmake_minimum_required(VERSION 2.8)
set(PROJECT_NAME "igdclient")
project(${PROJECT_NAME})

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "release")
endif()

if (NOT BUILD_SHARED_LIBS)
	set(BUILD_SHARED_LIBS "ON")
endif()

if (BUILD_SHARED_LIBS STREQUAL "ON")
	set(LIB_EXTENSION "so")
else()
	set(LIB_EXTENSION "a")
endif()


set(IGDCP_INCLUDE_PATH "${CMAKE_BINARY_DIR}/include")
set(IGDCP_PATH "${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE}")


# cmake 실행한 위치와 소스파일 위치가 동일한 경우(test_dms 폴더에서 빌드한 경우임)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
	set(IGDCP_INCLUDE_PATH "${CMAKE_BINARY_DIR}/../cmake.build/include")
	set(IGDCP_PATH "${CMAKE_BINARY_DIR}/../cmake.build/lib/${CMAKE_BUILD_TYPE}")

endif()


include_directories(${IGDCP_INCLUDE_PATH})

add_definitions(-D_POSIX -DMICROSTACK_NOTLS)
add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} ${IGDCP_PATH}/libigdclient.${LIB_EXTENSION})
target_link_libraries(${PROJECT_NAME} pthread)
