cmake_minimum_required (VERSION 2.6)

SET(rs_SRCS rapidshare_com.cpp ../dl/download_container.cpp)
SET(ul_SRCS uploaded_to.cpp ../dl/download_container.cpp)

SET_SOURCE_FILES_PROPERTIES(${rs_SRCS} PROPERTIES COMPILE_FLAGS -DIS_PLUGIN)
SET_SOURCE_FILES_PROPERTIES(${ul_SRCS} PROPERTIES COMPILE_FLAGS -DIS_PLUGIN)

find_package(CURL REQUIRED)
IF(CURL_FOUND)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs})
	ADD_LIBRARY(rapidshare.com SHARED ${rs_SRCS})
	ADD_LIBRARY(uploaded.to SHARED ${ul_SRCS})
	TARGET_LINK_LIBRARIES(rapidshare.com ${CURL_LIBRARIES})
	TARGET_LINK_LIBRARIES(uploaded.to ${CURL_LIBRARIES})
ENDIF(CURL_FOUND)

install(TARGETS rapidshare.com DESTINATION share/downloaddaemon/plugins)
install(TARGETS uploaded.to DESTINATION share/downloaddaemon/plugins)
