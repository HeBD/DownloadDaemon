cmake_minimum_required (VERSION 2.6)

add_subdirectory(plugins)
add_subdirectory(reconnect)

SET(daemon_SRCS downloaddaemon.cpp 
		tools/helperfunctions.cpp 
		tools/curl_callbacks.cpp 
		mgmt/global_management.cpp 
		mgmt/mgmt_thread.cpp
		dl/download_thread.cpp 
		dl/download.cpp 
		../lib/netpptk/netpptk.cpp 
		../lib/cfgfile/cfgfile.cpp
		dl/download_container.cpp
		 ../lib/crypt/md5.cpp)


find_package(Boost 1.36.0 REQUIRED COMPONENTS thread)
find_package(CURL REQUIRED)
IF(Boost_FOUND AND CURL_FOUND)
	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs})
	ADD_EXECUTABLE(DownloadDaemon ${daemon_SRCS})
	TARGET_LINK_LIBRARIES(DownloadDaemon ${Boost_LIBRARIES} ${CURL_LIBRARIES})
ENDIF(Boost_FOUND AND CURL_FOUND)

install(TARGETS DownloadDaemon DESTINATION bin/)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/../../etc/init.d/downloadd DESTINATION /etc/init.d)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../LICENCE DESTINATION share/downloaddaemon) 
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../etc/downloaddaemon/downloaddaemon.conf DESTINATION /etc/downloaddaemon)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../etc/downloaddaemon/routerinfo.conf DESTINATION /etc/downloaddaemon)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../etc/downloaddaemon/premium_accounts.conf DESTINATION /etc/downloaddaemon)
install(CODE "FILE(MAKE_DIRECTORY /var/downloads)")
