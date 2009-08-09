cmake_minimum_required (VERSION 2.6)

set(CMAKE_BUILD_TYPE Debug)

SET(ddclient_wx_SRCS	ddclient-wx_connect_dialog.cpp
			ddclient-wx.cpp  
			ddclient-wx_main.cpp
			../lib/netpptk/netpptk.cpp 
			../lib/cfgfile/cfgfile.cpp)

find_package(Boost REQUIRED COMPONENTS thread)
find_package(wxWidgets REQUIRED base core)

IF(Boost_FOUND AND wxWidgets_FOUND)
	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

	INCLUDE(${wxWidgets_USE_FILE})

	ADD_EXECUTABLE(ddclient-wx ${ddclient_wx_SRCS})
	TARGET_LINK_LIBRARIES(ddclient-wx ${Boost_LIBRARIES} ${wxWidgets_LIBRARIES})
ENDIF(Boost_FOUND AND wxWidgets_FOUND)

install(TARGETS ddclient-wx DESTINATION bin)

