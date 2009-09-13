cmake_minimum_required (VERSION 2.6)
project(ddclient)

SET(ddclient_SRCS ddclient.cpp ../lib/netpptk/netpptk.cpp)

SET_SOURCE_FILES_PROPERTIES(${ddclient_SRCS} PROPERTIES COMPILE_FLAGS -DNO_BOOST_THREAD)
ADD_EXECUTABLE(ddclient ${ddclient_SRCS})

install(TARGETS ddclient DESTINATION bin)
