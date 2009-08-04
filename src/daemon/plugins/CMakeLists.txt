cmake_minimum_required (VERSION 2.6)

SET(rs_SRCS rapidshare_com.cpp)
SET(ul_SRCS uploaded_to.cpp)

find_package(CURL REQUIRED)
IF(CURL_FOUND)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs})
	ADD_EXECUTABLE(rapidshare.com ${rs_SRCS})
	ADD_EXECUTABLE(uploaded.to ${ul_SRCS})
	TARGET_LINK_LIBRARIES(rapidshare.com ${CURL_LIBRARIES})
	TARGET_LINK_LIBRARIES(uploaded.to ${CURL_LIBRARIES})
ENDIF(CURL_FOUND)

install(TARGETS rapidshare.com DESTINATION /opt/downloaddaemon/plugins)
install(TARGETS uploaded.to DESTINATION /opt/downloaddaemon/plugins)
