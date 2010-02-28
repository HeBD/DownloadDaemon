cmake_minimum_required (VERSION 2.6)
project(ddclient-gui)

SET(ddclient_gui_SRCS	ddclient_gui.cpp
			main.cpp
			../include/netpptk/netpptk.cpp
			../include/crypt/md5.cpp
			../include/language/language.cpp)

SET(moc_headers		ddclient_gui.h)

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckTypeSize)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckCXXSourceCompiles)

CHECK_CXX_COMPILER_FLAG(-std=c++0x HAVE_STD_CXX0X)
IF(HAVE_STD_CXX0X)
	CHECK_INCLUDE_FILE_CXX(thread USE_STD_THREAD -std=c++0x)
ELSE(HAVE_STD_CXX0X)
	set(USE_STD_THREAD false)
ENDIF(HAVE_STD_CXX0X)
CONFIGURE_FILE(config.h.cmake ${CMAKE_BINARY_DIR}/include/config.h)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include/ ../include)


find_package(Qt4 COMPONENTS QtCore QtGui REQUIRED)
include(${QT_USE_FILE})
QT4_WRAP_CPP(ddclient_gui_MOC_SRCS ${moc_headers})
IF(NOT USE_STD_THREAD)
	find_package(Boost 1.36.0 REQUIRED COMPONENTS thread)
	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR} ../include)
	ADD_EXECUTABLE(ddclient-gui ${ddclient_gui_SRCS} ${ddclient_gui_MOC_SRCS})
	TARGET_LINK_LIBRARIES(ddclient-gui ${Boost_LIBRARIES} ${QT_LIBRARIES})
ELSE(NOT USE_STD_THREAD)
	ADD_EXECUTABLE(ddclient-gui ${ddclient_gui_SRCS} ${ddclient_gui_MOC_SRCS})
	SET_TARGET_PROPERTIES(ddclient-gui PROPERTIES COMPILE_FLAGS -std=c++0x)
	TARGET_LINK_LIBRARIES(ddclient-gui ${QT_LIBRARIES})
ENDIF(NOT USE_STD_THREAD)

install(TARGETS ddclient-gui DESTINATION bin)

#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/teufelchen_about.png DESTINATION share/ddclient-wx/img/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/logoDD.png DESTINATION share/ddclient-wx/img/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/1_connect.png DESTINATION share/ddclient-wx/img/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/2_add.png DESTINATION share/ddclient-wx/img/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/3_delete.png DESTINATION share/ddclient-wx/img/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/4_stop.png DESTINATION share/ddclient-wx/img/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/5_start.png DESTINATION share/ddclient-wx/img/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/6_up.png DESTINATION share/ddclient-wx/img/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/7_down.png DESTINATION share/ddclient-wx/img/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/8_configure.png DESTINATION share/ddclient-wx/img/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/9_activate.png DESTINATION share/ddclient-wx/img/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/9_deactivate.png DESTINATION share/ddclient-wx/img/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/10_delete_finished.png DESTINATION share/ddclient-wx/img/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/pixmaps/ddclient-wx.png DESTINATION share/pixmaps)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/pixmaps/ddclient-wx.xpm DESTINATION share/pixmaps)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/applications/ddclient-wx.desktop DESTINATION share/applications/)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/doc/ddclient-wx/copyright DESTINATION share/doc/ddclient-wx/)

