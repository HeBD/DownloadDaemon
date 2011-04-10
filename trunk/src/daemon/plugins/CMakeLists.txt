cmake_minimum_required (VERSION 2.6)

SET(plugins rapidshare_com.cpp   rapidshare.com 
            uploaded_to.cpp      uploaded.to 
            youtube_com.cpp      youtube.com
            netload_in.cpp       netload.in
            megaupload_com.cpp   megaupload.com
            netfolder_in.cpp     netfolder.in
            filefactory_com.cpp  filefactory.com
            depositfiles_com.cpp depositfiles.com
            hotfile_com.cpp      hotfile.com 
            fileserve_com.cpp    fileserve.com
            linksave_in.cpp      linksave.in 
            relink_us.cpp        relink.us
            files_mail_ru.cpp    files.mail.ru
            load_to.cpp          load.to
            zshare_net.cpp       zshare.net
            shragle_com.cpp      shragle.com
            bitload_com.cpp      bitload.com
            share_cx.cpp         share.cx
            filesonic_com.cpp    filesonic.com
            share-online_biz.cpp share-online.biz

)

project(DownloadDaemon-Plugins)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckTypeSize)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckCXXSourceCompiles)


#SET(DD_CONF_DIR "\"/etc/downloaddaemon/\"")
#SET(${DD_CONF_DIR}_OPTION "ON")

#CHECK_INCLUDE_FILE_CXX (syslog.h HAVE_SYSLOG_H)
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

CONFIGURE_FILE(../config.h.cmake ${CMAKE_BINARY_DIR}/include_downloadd-plugins/config.h)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include_downloadd-plugins/ ../../include)


CHECK_FUNCTION_EXISTS(popen HAVE_POPEN)
IF(NOT CYGWIN)
	add_definitions(-rdynamic)
ENDIF(NOT CYGWIN)


find_package(CURL REQUIRED)

IF(CURL_FOUND AND HAVE_POPEN)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRs} ${CMAKE_BINARY_DIR}/include)



	SET(link_libs ${CURL_LIBRARIES})
	IF(CYGWIN)
		SET(link_libs ${link_libs} -lDownloadDaemon)
	ENDIF(CYGWIN)

	IF(NOT USE_STD_THREAD)
		find_package(Boost 1.36.0 REQUIRED COMPONENTS thread)
		SET(link_libs ${link_libs} ${Boost_LIBRARIES})
		INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
	ELSE(NOT USE_STD_THREAD)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
		#SET_TARGET_PROPERTIES(${plugins} PROPERTIES COMPILE_FLAGS -std=c++0x)
	ENDIF(NOT USE_STD_THREAD)

	set(srcfile "")
	set(skip_next true)
	Message("-- The following Plugins have been found:")
	foreach(plg ${plugins})
		IF(NOT skip_next)
			SET(skip_next true)
			MESSAGE(" -- ${plg}")


			ADD_LIBRARY(${plg} MODULE ${srcfile})
			set_target_properties(${plg} PROPERTIES PREFIX lib SUFFIX .so)
			IF(CYGWIN)
				set_target_properties(${plg} PROPERTIES LINK_FLAGS "-L..")
			ENDIF(CYGWIN)
			IF(APPLE)
				set(CMAKE_MODULE_LINKER_FLAGS "-undefined suppress -flat_namespace")
			ENDIF(APPLE)
			
			TARGET_LINK_LIBRARIES(${plg} ${link_libs})

			install(TARGETS ${plg} DESTINATION share/downloaddaemon/plugins)

		ELSE(NOT skip_next)
			SET(skip_next false)
		ENDIF(NOT skip_next)
		set(srcfile ${plg})
	endforeach(plg)

	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../../share/downloaddaemon/plugins/captchadb/netload.in.tar.bz2 DESTINATION share/downloaddaemon/plugins/captchadb)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../../share/downloaddaemon/plugins/captchadb/megaupload.com.tar.bz2 DESTINATION share/downloaddaemon/plugins/captchadb)

	install(CODE "EXEC_PROGRAM(\"tar xjf \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/netload.in.tar.bz2 -C \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/ \")")


	#ADD_LIBRARY(rapidshare.com MODULE rapidshare_com.cpp)
	#ADD_LIBRARY(uploaded.tolastplg MODULE uploaded_to.cpp)
	#ADD_LIBRARY(youtube.com MODULE youtube_com.cpp)
	#ADD_LIBRARY(netload.in MODULE netload_in.cpp)
	#ADD_LIBRARY(megaupload.com MODULE megaupload_com.cpp)
	#ADD_LIBRARY(netfolder.in MODULE netfolder_in.cpp)
	#ADD_LIBRARY(filefactory.com MODULE filefactory_com.cpp)
	#ADD_LIBRARY(depositfiles.com MODULE depositfiles_com.cpp)
	#ADD_LIBRARY(hotfile.com MODULE hotfile_com.cpp)
	#ADD_LIBRARY(fileserve.com MODULE fileserve_com.cpp)
	#ADD_LIBRARY(linksave.in MODULE linksave_in.cpp)
	#ADD_LIBRARY(relink.us MODULE relink_us.cpp)
	#ADD_LIBRARY(files.mail.ru MODULE files_mail_ru.cpp)
	#ADD_LIBRARY(load.to MODULE load_to.cpp)
	#ADD_LIBRARY(zshare.net MODULE zshare_net.cpp)
	#ADD_LIBRARY(shragle.com MODULE shragle_com.cpp)
	#ADD_LIBRARY(bitload.com MODULE bitload_com.cpp)
	#ADD_LIBRARY(share.cx MODULE share_cx.cpp)
	#ADD_LIBRARY(filesonic.com MODULE filesonic_com.cpp)
	#ADD_LIBRARY(share-online.biz MODULE share-online_biz.cpp)




        #set_target_properties(${plugins} PROPERTIES PREFIX lib SUFFIX .so)

	#IF(CYGWIN)
	#	set_target_properties(${plugins} PROPERTIES LINK_FLAGS "-L..")
		#SET(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS} -L../ -lDownloadDaemon --enable-runtime-pseudo-reloc)
	#ENDIF(CYGWIN)

	#IF(APPLE)
	#	set(CMAKE_MODULE_LINKER_FLAGS "-undefined suppress -flat_namespace")
	#ENDIF(APPLE)
	
	#SET(link_libs ${CURL_LIBRARIES})
	#IF(CYGWIN)
	#	SET(link_libs ${link_libs} -lDownloadDaemon)
	#ENDIF(CYGWIN)
	
	#IF(NOT USE_STD_THREAD)
	#	find_package(Boost 1.36.0 REQUIRED COMPONENTS thread)
	#	SET(link_libs ${link_libs} ${Boost_LIBRARIES})
	#	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
	#ELSE(NOT USE_STD_THREAD)
	#	SET_TARGET_PROPERTIES(${plugins} PROPERTIES COMPILE_FLAGS -std=c++0x)
	#ENDIF(NOT USE_STD_THREAD)

	#TARGET_LINK_LIBRARIES(rapidshare.com ${link_libs})
	#TARGET_LINK_LIBRARIES(uploaded.to ${link_libs})
	#TARGET_LINK_LIBRARIES(youtube.com ${link_libs})
	#TARGET_LINK_LIBRARIES(netload.in ${link_libs})
	#TARGET_LINK_LIBRARIES(megaupload.com ${link_libs})
	#TARGET_LINK_LIBRARIES(netfolder.in ${link_libs})
	##TARGET_LINK_LIBRARIES(filefactory.com ${link_libs})
	#TARGET_LINK_LIBRARIES(depositfiles.com ${link_libs})
	#TARGET_LINK_LIBRARIES(hotfile.com ${link_libs})
	#TARGET_LINK_LIBRARIES(fileserve.com ${link_libs})
	#TARGET_LINK_LIBRARIES(linksave.in ${link_libs})
	#TARGET_LINK_LIBRARIES(relink.us ${link_libs})
	#TARGET_LINK_LIBRARIES(files.mail.ru ${link_libs})
	#TARGET_LINK_LIBRARIES(load.to ${link_libs})
	#TARGET_LINK_LIBRARIES(zshare.net ${link_libs})
	#TARGET_LINK_LIBRARIES(shragle.com ${link_libs})
	#TARGET_LINK_LIBRARIES(bitload.com ${link_libs})
	#TARGET_LINK_LIBRARIES(share.cx ${link_libs})
	#TARGET_LINK_LIBRARIES(filesonic.com ${link_libs})
	#TARGET_LINK_LIBRARIES(share-online.biz ${link_libs})

	##install(TARGETS ${plugins} DESTINATION share/downloaddaemon/plugins)



#	install(CODE "EXEC_PROGRAM(\"tar xjf \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/megaupload.com.tar.bz2 -C \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/ \")")


ENDIF(CURL_FOUND AND HAVE_POPEN)


