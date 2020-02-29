cmake_minimum_required(VERSION 2.8)
set(PROJECT_NAME "igdclient")
project(${PROJECT_NAME})

add_definitions(-D_POSIX -DMICROSTACK_NOTLS)

add_library(${PROJECT_NAME}
	IGD_AsyncServerSocket.c  IGD_Parsers.c     IGD_WebServer.c
	IGD_AsyncSocket.c        IGD_SSDPClient.c
	IGD_AsyncUDPSocket.c     IGD_WebClient.c   IGD_ControlPoint.c
)

SET_TARGET_PROPERTIES(${PROJECT_NAME}
	PROPERTIES
	SOVERSION 0
)

INSTALL(FILES
	IGD_Client.h
	IGD_WebServer.h
	IGD_AsyncSocket.h
	IGD_AsyncServerSocket.h
	IGD_Parsers.h
	IGD_ControlPoint.h
	IGD_ControlPointStructs.h
	DESTINATION ${CMAKE_BINARY_DIR}/include
)
