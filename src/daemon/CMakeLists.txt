cmake_minimum_required (VERSION 2.6)

add_subdirectory(plugins)
add_subdirectory(reconnect)

set(CMAKE_BUILD_TYPE Debug)

SET(daemon_SRCS downloaddaemon.cpp 
		tools/helperfunctions.cpp 
		tools/curl_callbacks.cpp 
		mgmt/global_management.cpp 
		mgmt/mgmt_thread.cpp
		dl/download_thread.cpp 
		dl/download.cpp 
		../lib/netpptk/netpptk.cpp 
		../lib/cfgfile/cfgfile.cpp
		dl/download_container.cpp)


find_package(Boost REQUIRED COMPONENTS thread)
find_package(CURL REQUIRED)
IF(Boost_FOUND AND CURL_FOUND)
	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs})
	ADD_EXECUTABLE(DownloadDaemon ${daemon_SRCS})
	TARGET_LINK_LIBRARIES(DownloadDaemon ${Boost_LIBRARIES} ${CURL_LIBRARIES})
ENDIF(Boost_FOUND AND CURL_FOUND)

add_custom_command(TARGET DownloadDaemon POST_BUILD 
	COMMAND mkdir ${PROJECT_BINARY_DIR}/bin ${PROJECT_BINARY_DIR}/share
	COMMAND mv ${PROJECT_BINARY_DIR}/DownloadDaemon ${PROJECT_BINARY_DIR}/bin/
	COMMAND

#install(TARGETS DownloadDaemon DESTINATION /opt/downloaddaemon/)
#install(FILES ../../program_data/LICENCE DESTINATION /opt/downloaddaemon) 
#install(FILES ../../program_data/conf/downloaddaemon.conf DESTINATION /opt/downloaddaemon/conf)
#install(FILES ../../program_data/downloaddaemon DESTINATION bin PERMISSIONS WORLD_EXECUTE WORLD_READ GROUP_EXECUTE GROUP_READ OWNER_EXECUTE OWNER_READ OWNER_WRITE)
#install(FILES ../../program_data/conf/routerinfo.conf DESTINATION /opt/downloaddaemon/conf)
#install(CODE "FILE(MAKE_DIRECTORY /opt/downloaddaemon/downloads)")


#install(FILES ../../program_data/downloads DESTINATION /opt/downloaddaemon/)
