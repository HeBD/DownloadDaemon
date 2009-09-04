cmake_minimum_required (VERSION 2.6)

find_package(CURL REQUIRED)
IF(CURL_FOUND)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs})
	ADD_LIBRARY(aquip_a-wlan-3 SHARED aquip_a-wlan-3.cpp ../../lib/mt_string/mt_string.cpp)
	TARGET_LINK_LIBRARIES(aquip_a-wlan-3 ${CURL_LIBRARIES})
ENDIF(CURL_FOUND)

install(TARGETS aquip_a-wlan-3 DESTINATION /opt/downloaddaemon/reconnect)

