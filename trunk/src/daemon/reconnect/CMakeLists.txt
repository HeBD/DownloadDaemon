cmake_minimum_required (VERSION 2.6)

find_package(CURL REQUIRED)
IF(CURL_FOUND)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs})
	ADD_LIBRARY(aquip_a-wlan-3 SHARED aquip_a-wlan-3.cpp)
	ADD_LIBRARY(belkin_generic SHARED belkin_generic.cpp)
	set_target_properties(aquip_a-wlan-3 belkin_generic PROPERTIES PREFIX lib SUFFIX .so)
	TARGET_LINK_LIBRARIES(aquip_a-wlan-3 ${CURL_LIBRARIES})
	TARGET_LINK_LIBRARIES(belkin_generic ${CURL_LIBRARIES})
ENDIF(CURL_FOUND)

install(TARGETS aquip_a-wlan-3 DESTINATION share/downloaddaemon/reconnect)
install(TARGETS belkin_generic DESTINATION share/downloaddaemon/reconnect)

