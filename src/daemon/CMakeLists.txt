cmake_minimum_required (VERSION 2.6)

add_subdirectory(plugins)

SET(daemon_SRCS downloaddaemon.cpp 
		tools/helperfunctions.cpp 
		tools/curl_callbacks.cpp 
		mgmt/global_management.cpp 
		mgmt/mgmt_thread.cpp
		dl/download.cpp 
		dl/recursive_parser.cpp
		../include/netpptk/netpptk.cpp 
		../include/cfgfile/cfgfile.cpp
		dl/download_container.cpp
		dl/package_container.cpp
		dl/plugin_container.cpp
		dl/package_extractor.cpp
		 ../include/crypt/md5.cpp
		reconnect/reconnect_parser.cpp
		../include/crypt/AES/aes_cbc.cpp
		../include/crypt/AES/aes_cfb.cpp
		../include/crypt/AES/aes_core.cpp
		../include/crypt/base64.cpp)

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckTypeSize)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckCXXSourceCompiles)

SET(GENERAL_LIKER_FLAGS "")

IF(NOT DD_CONF_DIR)
	SET(DD_CONF_DIR "/etc/downloaddaemon/")
ENDIF(NOT DD_CONF_DIR)
SET(DD_CONF_DIR_OPT "\"${DD_CONF_DIR}\"")
SET(${DD_CONF_DIR_OPT}_OPTION "ON")



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

SET(DOWNLOADDAEMON true)
IF(NOT VERSION)
	FIND_PACKAGE(Subversion)
	IF(Subversion_FOUND)
		Subversion_WC_INFO(${PROJECT_SOURCE_DIR} svn)
		SET(VERSION r${svn_WC_REVISION})
	ELSE(Subversion_FOUND)
		SET(VERSION "unknown")
	ENDIF(Subversion_FOUND)
ENDIF(NOT VERSION)
SET(DOWNLOADDAEMON_VERSION "ON")

CHECK_FUNCTION_EXISTS(initgroups HAVE_INITGROUPS)

# If -DBACKTRACE_ON_CRASH is used, this checks for the requirements and maybe disables it, if it's not available
IF(BACKTRACE_ON_CRASH)
	CHECK_INCLUDE_FILE_CXX(execinfo.h BACKTRACE_ON_CRASH)
	IF(BACKTRACE_ON_CRASH)
		CHECK_FUNCTION_EXISTS(backtrace BACKTRACE_ON_CRASH)
	ENDIF(BACKTRACE_ON_CRASH)
	IF(NOT BACKTRACE_ON_CRASH)
		message("You specified -DBACKTRACE_ON_CRASH, however, you are either missing the execinfo.h header, or the backtrace() function. The option has beed siabled automatically.")
	ELSE(NOT BACKTRACE_ON_CRASH)
	SET(GENERAL_LINKER_FLAGS "${GENERAL_LINKER_FLAGS} -rdynamic")
	ENDIF(NOT BACKTRACE_ON_CRASH)
ENDIF(BACKTRACE_ON_CRASH)

CONFIGURE_FILE(config.h.cmake ${CMAKE_BINARY_DIR}/include/config.h)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include/ ../include)


CHECK_FUNCTION_EXISTS(popen HAVE_POPEN)


find_package(CURL REQUIRED)


IF(CURL_FOUND AND HAVE_POPEN)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs})
	IF(NOT USE_STD_THREAD)
		find_package(Boost 1.36.0 REQUIRED COMPONENTS thread)
		IF(Boost_FOUND)
			INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
			ADD_EXECUTABLE(DownloadDaemon ${daemon_SRCS})
			IF(BACKTRACE_ON_CRASH)
				SET_TARGET_PROPERTIES(DownloadDaemon PROPERTIES LINK_FLAGS ${GENERAL_LINKER_FLAGS})
			ENDIF(BACKTRACE_ON_CRASH)
			TARGET_LINK_LIBRARIES(DownloadDaemon ${Boost_LIBRARIES} ${CURL_LIBRARIES} ${CMAKE_DL_LIBS})
		ENDIF(Boost_FOUND)
	ELSE(NOT USE_STD_THREAD)
		ADD_EXECUTABLE(DownloadDaemon ${daemon_SRCS})
		IF(BACKTRACE_ON_CRASH)
			SET_TARGET_PROPERTIES(DownloadDaemon PROPERTIES COMPILE_FLAGS -std=c++0x LINK_FLAGS ${GENERAL_LINKER_FLAGS})
		ELSE(BACKTRACE_ON_CRASH)
			SET_TARGET_PROPERTIES(DownloadDaemon PROPERTIES COMPILE_FLAGS -std=c++0x)
		ENDIF(BACKTRACE_ON_CRASH)
		TARGET_LINK_LIBRARIES(DownloadDaemon ${CURL_LIBRARIES} ${CMAKE_DL_LIBS})
	ENDIF(NOT USE_STD_THREAD)
ENDIF(CURL_FOUND AND HAVE_POPEN)


install(TARGETS DownloadDaemon DESTINATION bin/)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/../../etc/init.d/downloadd DESTINATION /etc/init.d)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../LICENCE DESTINATION share/downloaddaemon) 
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../etc/downloaddaemon/downloaddaemon.conf DESTINATION ${DD_CONF_DIR} PERMISSIONS OWNER_READ OWNER_WRITE)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../etc/downloaddaemon/routerinfo.conf DESTINATION ${DD_CONF_DIR} PERMISSIONS OWNER_READ OWNER_WRITE)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../etc/downloaddaemon/premium_accounts.conf DESTINATION ${DD_CONF_DIR} PERMISSIONS OWNER_READ OWNER_WRITE)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../share/downloaddaemon/reconnect DESTINATION share/downloaddaemon PATTERN ".svn" EXCLUDE)
