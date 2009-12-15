cmake_minimum_required (VERSION 2.6)
project(ddclient-wx)

SET(ddclient_wx_SRCS	ddclient-wx_about_dialog.cpp
			ddclient-wx_configure_dialog.cpp
			ddclient-wx.cpp
			ddclient-wx_picture.cpp
			ddclient-wx_add_dialog.cpp
			ddclient-wx_connect_dialog.cpp
			ddclient-wx_main.cpp
			ddclient-wx_delete_dialog.cpp
			../lib/netpptk/netpptk.cpp
			../lib/crypt/md5.cpp)

find_package(Boost 1.36.0 REQUIRED COMPONENTS thread)
find_package(wxWidgets REQUIRED base core richtext)

IF(Boost_FOUND AND wxWidgets_FOUND)
	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

	INCLUDE(${wxWidgets_USE_FILE})

	ADD_EXECUTABLE(ddclient-wx ${ddclient_wx_SRCS})
	TARGET_LINK_LIBRARIES(ddclient-wx ${Boost_LIBRARIES} ${wxWidgets_LIBRARIES})
ENDIF(Boost_FOUND AND wxWidgets_FOUND)

install(TARGETS ddclient-wx DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/teufelchen_about.png DESTINATION share/ddclient-wx/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/logoDD.png DESTINATION share/ddclient-wx/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/1_connect.png DESTINATION share/ddclient-wx/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/2_add.png DESTINATION share/ddclient-wx/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/3_delete.png DESTINATION share/ddclient-wx/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/4_stop.png DESTINATION share/ddclient-wx/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/5_start.png DESTINATION share/ddclient-wx/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/6_up.png DESTINATION share/ddclient-wx/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/7_down.png DESTINATION share/ddclient-wx/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/8_configure.png DESTINATION share/ddclient-wx/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/9_activate.png DESTINATION share/ddclient-wx/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/ddclient-wx/img/9_deactivate.png DESTINATION share/ddclient-wx/img/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/pixmaps/ddclient-wx.png DESTINATION share/pixmaps)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/pixmaps/ddclient-wx.xpm DESTINATION share/pixmaps)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/applications/ddclient-wx.desktop DESTINATION share/applications/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../share/doc/ddclient-wx/copyright DESTINATION share/doc/ddclient-wx/)

