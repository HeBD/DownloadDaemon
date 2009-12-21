cmake_minimum_required (VERSION 2.6)

find_package(CURL REQUIRED)
find_package(Boost 1.36.0 REQUIRED COMPONENTS thread)
IF(Boost_FOUND AND CURL_FOUND)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs})
	
	ADD_LIBRARY(rapidshare.com SHARED rapidshare_com.cpp)
	ADD_LIBRARY(uploaded.to SHARED uploaded_to.cpp)
	TARGET_LINK_LIBRARIES(rapidshare.com ${CURL_LIBRARIES} ${Boost_LIBRARIES})
	TARGET_LINK_LIBRARIES(uploaded.to ${CURL_LIBRARIES} ${Boost_LIBRARIES})
	set_target_properties(rapidshare.com uploaded.to PROPERTIES PREFIX lib SUFFIX .so)
	# !!!! add new plugins here !!!!



	install(TARGETS rapidshare.com DESTINATION share/downloaddaemon/plugins)
	install(TARGETS uploaded.to DESTINATION share/downloaddaemon/plugins)
ENDIF(Boost_FOUND AND CURL_FOUND)


