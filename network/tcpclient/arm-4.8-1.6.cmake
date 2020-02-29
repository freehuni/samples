SET(TOOLCHAIN_PATH /opt/toolchains/stbgcc-4.8-1.6/bin)

SET(CMAKE_C_COMPILER  ${TOOLCHAIN_PATH}/arm-linux-gcc)
SET(CMAKE_CXX_COMPILER  ${TOOLCHAIN_PATH}/arm-linux-g++)
SET(CMAKE_LINKER    ${TOOLCHAIN_PATH}/arm-linux-ld)
SET(CMAKE_NM    ${TOOLCHAIN_PATH}/arm-linux-nm)
SET(CMAKE_OBJCOPY   ${TOOLCHAIN_PATH}/arm-linux-objcopy)
SET(CMAKE_OBJDUMP   ${TOOLCHAIN_PATH}/arm-linux-objdump)
SET(CMAKE_RANLIB    ${TOOLCHAIN_PATH}/arm-linux-ranlib)
