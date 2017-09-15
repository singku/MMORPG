SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
FIND_LIBRARY( TCMALLOC_LIBRARY
	NAMES tcmalloc
	PATHS /usr/local/lib
	DOC "The tcmalloc library")

IF (TCMALLOC_LIBRARY)
	SET( TCMALLOC_FOUND 1 CACHE STRING "Set to 1 if tcmalloc is found, 0 otherwise")
ELSE (TCMALLOC_LIBRARY)
	SET( TCMALLOC_FOUND 0 CACHE STRING "Set to 1 if tcmalloc is found, 0 otherwise")
ENDIF (TCMALLOC_LIBRARY)

MARK_AS_ADVANCED( TCMALLOC_FOUND )

IF (TCMALLOC_FOUND)
	MESSAGE(STATUS "找到了 tcmalloc 库")
ELSE (TCMALLOC_FOUND)
	MESSAGE(FATAL_ERROR "没有找到 tcmalloc 库 :请安装它, 建议源码安装")
ENDIF (TCMALLOC_FOUND)

