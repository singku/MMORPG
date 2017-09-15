SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
FIND_LIBRARY( UUID_LIBRARY
	NAMES uuid
	PATHS /usr/lib
	DOC "The uuid library")

IF (UUID_LIBRARY)
	SET( UUID_FOUND 1 CACHE STRING "Set to 1 if uuid is found, 0 otherwise")
ELSE (UUID_LIBRARY)
	SET( UUID_FOUND 0 CACHE STRING "Set to 1 if uuid is found, 0 otherwise")
ENDIF (UUID_LIBRARY)

MARK_AS_ADVANCED( UUID_FOUND )

IF (UUID_FOUND)
	MESSAGE(STATUS "找到了 uuid 库")
ELSE (UUID_FOUND)
	MESSAGE(FATAL_ERROR "没有找到 uuid 库 :请安装它: 建议源码安装 (带上 -fPIC 参数编译)")
ENDIF (UUID_FOUND)

