cmake_minimum_required (VERSION 2.6)
project(ddclient-wx)

SET(ddclient_gui_SRCS	ddclient_gui.cpp
			main.cpp
			../include/netpptk/netpptk.cpp
			../include/crypt/md5.cpp
			../include/language/language.cpp)

SET(moc_headers		ddclient_gui.h)


find_package(Boost 1.36.0 REQUIRED COMPONENTS thread)
find_package(Qt4 COMPONENTS QtCore QtGui REQUIRED)
include(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR} ../include)
QT4_WRAP_CPP(ddclient_gui_MOC_SRCS ${moc_headers})

ADD_EXECUTABLE(ddclient-gui ${ddclient_gui_SRCS} ${ddclient_gui_MOC_SRCS})
TARGET_LINK_LIBRARIES(ddclient-gui ${Boost_LIBRARIES} ${QT_LIBRARIES})

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

