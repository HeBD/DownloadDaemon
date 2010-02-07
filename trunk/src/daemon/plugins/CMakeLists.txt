cmake_minimum_required (VERSION 2.6)

find_package(CURL REQUIRED)
find_package(Boost 1.36.0 REQUIRED COMPONENTS thread)
IF(Boost_FOUND AND CURL_FOUND)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs})
	
	ADD_LIBRARY(rapidshare.com MODULE rapidshare_com.cpp)
	ADD_LIBRARY(uploaded.to MODULE uploaded_to.cpp)
	ADD_LIBRARY(youtube.com MODULE youtube_com.cpp)
	ADD_LIBRARY(netload.in MODULE netload_in.cpp)

	TARGET_LINK_LIBRARIES(rapidshare.com ${CURL_LIBRARIES} ${Boost_LIBRARIES})
	TARGET_LINK_LIBRARIES(uploaded.to ${CURL_LIBRARIES} ${Boost_LIBRARIES})
	TARGET_LINK_LIBRARIES(youtube.com ${CURL_LIBRARIES} ${Boost_LIBRARIES})
	TARGET_LINK_LIBRARIES(netload.in ${CURL_LIBRARIES} ${Boost_LIBRARIES})
	set_target_properties(rapidshare.com uploaded.to youtube.com netload.in PROPERTIES PREFIX lib SUFFIX .so)
	# !!!! add new plugins here !!!!



	install(TARGETS rapidshare.com DESTINATION share/downloaddaemon/plugins)
	install(TARGETS uploaded.to DESTINATION share/downloaddaemon/plugins)
	install(TARGETS youtube.com DESTINATION share/downloaddaemon/plugins)
	install(TARGETS netload.in DESTINATION share/downloaddaemon/plugins)	
ENDIF(Boost_FOUND AND CURL_FOUND)


