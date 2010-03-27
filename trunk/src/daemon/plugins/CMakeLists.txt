cmake_minimum_required (VERSION 2.6)

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
	#message ("-- WARNING: You do not have uint64_t. Using double instead. This might cause problems in rare cases")
	#set(HAVE_STD_INT false)
ENDIF(NOT HAVE_UINT64_T)

CONFIGURE_FILE(../config.h.cmake ${CMAKE_BINARY_DIR}/include/config.h)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include/ ../include)


CHECK_FUNCTION_EXISTS(popen HAVE_POPEN)

find_package(CURL REQUIRED)

IF(CURL_FOUND AND HAVE_POPEN)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs} ${CMAKE_BINARY_DIR}/include)

	ADD_LIBRARY(rapidshare.com MODULE rapidshare_com.cpp)
	ADD_LIBRARY(uploaded.to MODULE uploaded_to.cpp)
	ADD_LIBRARY(youtube.com MODULE youtube_com.cpp)
	ADD_LIBRARY(netload.in MODULE netload_in.cpp)
	ADD_LIBRARY(megaupload.com MODULE megaupload_com.cpp)
	ADD_LIBRARY(netfolder.in MODULE netfolder_in.cpp)

	set_target_properties(rapidshare.com uploaded.to youtube.com netload.in megaupload.com netfolder.in PROPERTIES PREFIX lib SUFFIX .so)
	# !!!! add new plugins here !!!!

	IF(USE_STD_THREAD)
		SET_TARGET_PROPERTIES(rapidshare.com uploaded.to youtube.com netload.in megaupload.com netfolder.in PROPERTIES COMPILE_FLAGS -std=c++0x)
		TARGET_LINK_LIBRARIES(rapidshare.com ${CURL_LIBRARIES})
		TARGET_LINK_LIBRARIES(uploaded.to ${CURL_LIBRARIES})
		TARGET_LINK_LIBRARIES(youtube.com ${CURL_LIBRARIES})
		TARGET_LINK_LIBRARIES(netload.in ${CURL_LIBRARIES})
		TARGET_LINK_LIBRARIES(megaupload.com ${CURL_LIBRARIES})
		TARGET_LINK_LIBRARIES(netfolder.in ${CURL_LIBRARIES})
	ELSE(USE_STD_THREAD)
		find_package(Boost 1.36.0 REQUIRED COMPONENTS thread)
		IF(Boost_FOUND)
			TARGET_LINK_LIBRARIES(rapidshare.com ${CURL_LIBRARIES} ${Boost_LIBRARIES})
			TARGET_LINK_LIBRARIES(uploaded.to ${CURL_LIBRARIES} ${Boost_LIBRARIES})
			TARGET_LINK_LIBRARIES(youtube.com ${CURL_LIBRARIES} ${Boost_LIBRARIES})
			TARGET_LINK_LIBRARIES(netload.in ${CURL_LIBRARIES} ${Boost_LIBRARIES})
			TARGET_LINK_LIBRARIES(megaupload.com ${CURL_LIBRARIES} ${Boost_LIBRARIES})
			TARGET_LINK_LIBRARIES(netfolder.in ${CURL_LIBRARIES} ${Boost_LIBRARIES})
		ENDIF(Boost_FOUND)
	ENDIF(USE_STD_THREAD)



	install(TARGETS rapidshare.com DESTINATION share/downloaddaemon/plugins)
	install(TARGETS uploaded.to DESTINATION share/downloaddaemon/plugins)
	install(TARGETS youtube.com DESTINATION share/downloaddaemon/plugins)
	install(TARGETS netload.in DESTINATION share/downloaddaemon/plugins)
	install(TARGETS megaupload.com DESTINATION share/downloaddaemon/plugins)
	install(TARGETS netfolder.in DESTINATION share/downloaddaemon/plugins)


	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../../share/downloaddaemon/plugins/captchadb/netload.in.tar.bz2 DESTINATION share/downloaddaemon/plugins/captchadb)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../../share/downloaddaemon/plugins/captchadb/megaupload.com.tar.bz2 DESTINATION share/downloaddaemon/plugins/captchadb)

	install(CODE "EXEC_PROGRAM(\"tar xjf \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/netload.in.tar.bz2 -C \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/ \")")
	install(CODE "EXEC_PROGRAM(\"tar xjf \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/megaupload.com.tar.bz2 -C \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/ \")")


ENDIF(CURL_FOUND AND HAVE_POPEN)


