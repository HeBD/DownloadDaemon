cmake_minimum_required (VERSION 2.6)
project(ddconsole)

SET(ddconsole_SRCS ddconsole.cpp ../lib/netpptk/netpptk.cpp)

SET_SOURCE_FILES_PROPERTIES(${ddconsole_SRCS} PROPERTIES COMPILE_FLAGS -DNO_BOOST_THREAD)
ADD_EXECUTABLE(ddconsole ${ddconsole_SRCS})

install(TARGETS ddconsole DESTINATION bin)
