cmake_minimum_required (VERSION 2.6)

find_package(CURL REQUIRED)
IF(CURL_FOUND)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs})
	ADD_LIBRARY(rapidshare.com SHARED rapidshare_com.cpp)
	ADD_LIBRARY(uploaded.to SHARED uploaded_to.cpp)
	# !!!! add new plugins here !!!!
	TARGET_LINK_LIBRARIES(rapidshare.com ${CURL_LIBRARIES})
	TARGET_LINK_LIBRARIES(uploaded.to ${CURL_LIBRARIES})
ENDIF(CURL_FOUND)

install(TARGETS rapidshare.com DESTINATION share/downloaddaemon/plugins)
install(TARGETS uploaded.to DESTINATION share/downloaddaemon/plugins)
