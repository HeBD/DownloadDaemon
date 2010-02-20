cmake_minimum_required (VERSION 2.6)


OPTION(USE_STD_THREAD "Use C++0x's std::thread implementation instead of boost::thread library" OFF)
SET(DD_CONF_DIR "\"/etc/downloaddaemon/\"")
SET(${DD_CONF_DIR}_OPTION "ON")
CONFIGURE_FILE(../config.h.cmake ${CMAKE_BINARY_DIR}/include/config.h)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include/ ../include)

find_package(CURL REQUIRED)

IF(CURL_FOUND)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs} ${CMAKE_BINARY_DIR}/include)

	ADD_LIBRARY(rapidshare.com MODULE rapidshare_com.cpp)
	ADD_LIBRARY(uploaded.to MODULE uploaded_to.cpp)
	ADD_LIBRARY(youtube.com MODULE youtube_com.cpp)
	ADD_LIBRARY(netload.in MODULE netload_in.cpp)
	ADD_LIBRARY(megaupload.com MODULE megaupload_com.cpp)

	set_target_properties(rapidshare.com uploaded.to youtube.com netload.in megaupload.com PROPERTIES PREFIX lib SUFFIX .so)
	# !!!! add new plugins here !!!!

	IF(USE_STD_THREAD)
		SET_TARGET_PROPERTIES(rapidshare.com uploaded.to youtube.com netload.in megaupload.com PROPERTIES COMPILE_FLAGS -std=c++0x)
		TARGET_LINK_LIBRARIES(rapidshare.com ${CURL_LIBRARIES})
		TARGET_LINK_LIBRARIES(uploaded.to ${CURL_LIBRARIES})
		TARGET_LINK_LIBRARIES(youtube.com ${CURL_LIBRARIES})
		TARGET_LINK_LIBRARIES(netload.in ${CURL_LIBRARIES})
		TARGET_LINK_LIBRARIES(megaupload.com ${CURL_LIBRARIES})
	ELSE(USE_STD_THREAD)
		find_package(Boost 1.36.0 REQUIRED COMPONENTS thread)
		IF(Boost_FOUND)
			TARGET_LINK_LIBRARIES(rapidshare.com ${CURL_LIBRARIES} ${Boost_LIBRARIES})
			TARGET_LINK_LIBRARIES(uploaded.to ${CURL_LIBRARIES} ${Boost_LIBRARIES})
			TARGET_LINK_LIBRARIES(youtube.com ${CURL_LIBRARIES} ${Boost_LIBRARIES})
			TARGET_LINK_LIBRARIES(netload.in ${CURL_LIBRARIES} ${Boost_LIBRARIES})
			TARGET_LINK_LIBRARIES(megaupload.com ${CURL_LIBRARIES} ${Boost_LIBRARIES})
		ENDIF(Boost_FOUND)
	ENDIF(USE_STD_THREAD)



	install(TARGETS rapidshare.com DESTINATION share/downloaddaemon/plugins)
	install(TARGETS uploaded.to DESTINATION share/downloaddaemon/plugins)
	install(TARGETS youtube.com DESTINATION share/downloaddaemon/plugins)
	install(TARGETS netload.in DESTINATION share/downloaddaemon/plugins)
	install(TARGETS megaupload.com DESTINATION share/downloaddaemon/plugins)


	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../../share/downloaddaemon/plugins/captchadb/netload.in.tar.bz2 DESTINATION share/downloaddaemon/plugins/captchadb)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../../share/downloaddaemon/plugins/captchadb/megaupload.com.tar.bz2 DESTINATION share/downloaddaemon/plugins/captchadb)

	install(CODE "EXEC_PROGRAM(\"tar xjf \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/netload.in.tar.bz2 -C \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/ \")")
	install(CODE "EXEC_PROGRAM(\"tar xjf \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/megaupload.com.tar.bz2 -C \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/ \")")


ENDIF(CURL_FOUND)


