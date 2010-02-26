cmake_minimum_required (VERSION 2.6)

add_subdirectory(plugins)

SET(daemon_SRCS downloaddaemon.cpp 
		tools/helperfunctions.cpp 
		tools/curl_callbacks.cpp 
		mgmt/global_management.cpp 
		mgmt/mgmt_thread.cpp
		dl/download_thread.cpp 
		dl/download.cpp 
		dl/recursive_parser.cpp
		../include/netpptk/netpptk.cpp 
		../include/cfgfile/cfgfile.cpp
		dl/download_container.cpp
		dl/package_container.cpp
		 ../include/crypt/md5.cpp
		reconnect/reconnect_parser.cpp)

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckTypeSize)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckCXXSourceCompiles)

SET(DD_CONF_DIR "\"/etc/downloaddaemon/\"")
SET(${DD_CONF_DIR}_OPTION "ON")



CHECK_INCLUDE_FILE_CXX (syslog.h HAVE_SYSLOG_H)
CHECK_CXX_COMPILER_FLAG(-std=c++0x HAVE_STD_CXX0X)

IF(HAVE_STD_CXX0X)
	CHECK_INCLUDE_FILE_CXX(thread USE_STD_THREAD -std=c++0x)
ELSE(HAVE_STD_CXX0X)
	set(USE_STD_THREAD false)
ENDIF(HAVE_STD_CXX0X)

CHECK_CXX_SOURCE_COMPILES("#include <stdint.h>\nint main() { uint64_t asdf = 12345; return 0; }" HAVE_UINT64_T)


IF(NOT HAVE_UINT64_T)
	message ("-- WARNING: You do not have uint64_t. Using double instead. This might cause problems in rare cases")
	set(HAVE_STD_INT false)
ENDIF(NOT HAVE_UINT64_T)

CONFIGURE_FILE(config.h.cmake ${CMAKE_BINARY_DIR}/include/config.h)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include/ ../include)


CHECK_FUNCTION_EXISTS(popen HAVE_POPEN)


find_package(CURL REQUIRED)
#IF(Boost_FOUND AND CURL_FOUND)
IF(CURL_FOUND AND HAVE_POPEN)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs})
	IF(NOT USE_STD_THREAD)
		find_package(Boost 1.36.0 REQUIRED COMPONENTS thread)
		IF(Boost_FOUND)
			INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
			ADD_EXECUTABLE(DownloadDaemon ${daemon_SRCS})
			TARGET_LINK_LIBRARIES(DownloadDaemon ${Boost_LIBRARIES} ${CURL_LIBRARIES})
		ENDIF(Boost_FOUND)
	ELSE(NOT USE_STD_THREAD)
		ADD_EXECUTABLE(DownloadDaemon ${daemon_SRCS})
		SET_TARGET_PROPERTIES(DownloadDaemon PROPERTIES COMPILE_FLAGS -std=c++0x)
		TARGET_LINK_LIBRARIES(DownloadDaemon ${CURL_LIBRARIES})
	ENDIF(NOT USE_STD_THREAD)
ENDIF(CURL_FOUND AND HAVE_POPEN)
#ENDIF(Boost_FOUND AND CURL_FOUND)

install(TARGETS DownloadDaemon DESTINATION bin/)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/../../etc/init.d/downloadd DESTINATION /etc/init.d)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../LICENCE DESTINATION share/downloaddaemon) 
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../etc/downloaddaemon/downloaddaemon.conf DESTINATION /etc/downloaddaemon PERMISSIONS OWNER_READ OWNER_WRITE)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../etc/downloaddaemon/routerinfo.conf DESTINATION /etc/downloaddaemon PERMISSIONS OWNER_READ OWNER_WRITE)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../etc/downloaddaemon/premium_accounts.conf DESTINATION /etc/downloaddaemon PERMISSIONS OWNER_READ OWNER_WRITE)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../share/downloaddaemon/reconnect DESTINATION share/downloaddaemon PATTERN ".svn" EXCLUDE)
