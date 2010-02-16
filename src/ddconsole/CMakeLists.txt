cmake_minimum_required (VERSION 2.6)
project(ddconsole)

SET(ddconsole_SRCS ddconsole.cpp ../include/netpptk/netpptk.cpp ../include/crypt/md5.cpp)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include ../include)
SET_SOURCE_FILES_PROPERTIES(${ddconsole_SRCS} PROPERTIES COMPILE_FLAGS -DNO_BOOST_THREAD)
ADD_EXECUTABLE(ddconsole ${ddconsole_SRCS})

install(TARGETS ddconsole DESTINATION bin)
