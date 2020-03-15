# 사용법:
# 1. ToolChain PATH와 ToolChain binary 설정
# 2. cmake -DCMAKE_TOOLCHAIN_FILE=<toolchain_template>.cmake .

SET(TOOLCHAIN_PATH /opt/toolchains/stbgcc-4.5.4-2.6/bin)

SET(CMAKE_C_COMPILER  	${TOOLCHAIN_PATH}/mipsel-linux-uclibc-gcc)
SET(CMAKE_CXX_COMPILER	${TOOLCHAIN_PATH}/mipsel-linux-uclibc-g++)
SET(CMAKE_LINKER    		${TOOLCHAIN_PATH}/mipsel-linux-uclibc-ld)
SET(CMAKE_NM    			${TOOLCHAIN_PATH}/mipsel-linux-uclibc-nm)
SET(CMAKE_OBJCOPY   		${TOOLCHAIN_PATH}/mipsel-linux-uclibc-objcopy)
SET(CMAKE_OBJDUMP   		${TOOLCHAIN_PATH}/mipsel-linux-uclibc-objdump)
SET(CMAKE_RANLIB    		${TOOLCHAIN_PATH}/mipsel-linux-uclibc-ranlib)
