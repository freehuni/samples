cmake_minimum_required(VERSION 2.8)
project(igdclient_root)

# ========= default configuration =====================
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "release")
endif()

# shared library output path
if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
endif()

# static library output path
if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
endif()

if (NOT BUILD_SHARED_LIBS)
	set(BUILD_SHARED_LIBS "ON")
endif()


# set(CMAKE_C_FLAGS "-m32")
# set(CMAKE_CXX_FLAGS "-m32")

# All libraries ========================================
add_subdirectory(src)
#add_subdirectory(sample)

# Distclean target =====================================
add_custom_target(distclean
	COMMAND make clean
	COMMAND rm CMakeCache.txt
	COMMAND rm -rf CMakeFiles
	COMMAND rm -rf ${CMAKE_BINARY_DIR}/include
	COMMAND rm -rf ${CMAKE_BINARY_DIR}/lib
	COMMENT "clean-up completed!"
)
