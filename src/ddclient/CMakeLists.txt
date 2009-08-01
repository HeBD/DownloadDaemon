cmake_minimum_required (VERSION 2.6)
project(ddclient)

SET(ddclient_SRCS ddclient.cpp ../lib/netpptk/netpptk.cpp)


find_package(Boost REQUIRED COMPONENTS thread)

IF(Boost_FOUND)
	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
	ADD_EXECUTABLE(ddclient ${ddclient_SRCS})
	TARGET_LINK_LIBRARIES(ddclient ${Boost_LIBRARIES})
ENDIF(Boost_FOUND)

install(TARGETS ddclient DESTINATION bin)
