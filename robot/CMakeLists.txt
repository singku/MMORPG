cmake_minimum_required(VERSION 2.6)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

#find_package(taomee REQUIRED)
#find_package(taomee++ REQUIRED)
FIND_PACKAGE(glib REQUIRED)
#find_package(libxml2 REQUIRED)
#find_package(AsncServ REQUIRED)
FIND_PACKAGE(protobuf REQUIRED)
#find_package(uuid REQUIRED)


SET(CMAKE_USE_RELATIVE_PATHS  true )


SET(PROTO_DIR ${CMAKE_SOURCE_DIR}/proto)
SET(PROTO_PB_DIR ${CMAKE_SOURCE_DIR}/proto/out)
AUX_SOURCE_DIRECTORY(${PROTO_PB_DIR} PB_SRC_LIST)


SET(SRC_LIST 
${PB_SRC_LIST}
header.pb.cc
common.pb.cc
pb0x00.pb.cc
pb0x01.pb.cc
fileutils.cc
pb_master.cc
config.cc
client.cc
robot.cc
main.cc
cmd_msg.cc
mencrypt.cc
)
INCLUDE_DIRECTORIES(${PROTO_PB_DIR})
INCLUDE_DIRECTORIES(${GLIB_INCLUDE_DIR})


SET(BIN_NAME "robot")
#ADD_DEFINITIONS( "-g -DDEBUG -DASSERVER -Wall -MMD -funroll-loops -fPIC" )
ADD_DEFINITIONS( "-g -DDEBUG -Wall -MMD -funroll-loops -fPIC" )
ADD_EXECUTABLE(${BIN_NAME} ${SRC_LIST})
TARGET_LINK_LIBRARIES (${BIN_NAME} ${GLIB_LIBRARY} "taomee" "ssl" "glog" "gflags" "gthread-2.0" "protobuf" "pthread")
