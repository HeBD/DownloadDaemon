cmake_minimum_required (VERSION 2.6)

SET(plugins rapidshare_com.cpp				rapidshare.com 
                uploaded_net.cpp			uploaded.net
		youtube_com.cpp				youtube.com
		netload_in.cpp				netload.in
		netfolder_in.cpp			netfolder.in
		filefactory_com.cpp			filefactory.com
		depositfiles_com.cpp			depositfiles.com
		hotfile_com.cpp				hotfile.com 
		fileserve_com.cpp			fileserve.com
		linksave_in.cpp				linksave.in 
		relink_us.cpp				relink.us
		files_mail_ru.cpp			files.mail.ru
		load_to.cpp				load.to
		zshare_net.cpp				zshare.net
		shragle_com.cpp				shragle.com
		bitload_com.cpp				bitload.com
		filesonic_com.cpp			filesonic.com
		share-online_biz.cpp			share-online.biz
		x7_to.cpp				x7.to
		duckload_com.cpp			duckload.com
		safelinking_net.cpp			safelinking.net
		serienjunkies_org.cpp			serienjunkies.org
		hd-bb_org.cpp				hd-bb.org
		iload_to.cpp				iload.to
		bierdopje_com.cpp			bierdopje.com
		multiupload_com.cpp			multiupload.com
		ifile_it.cpp				ifile.it
		linksafe_me.cpp				linksafe.me
		mediafire_com.cpp			mediafire.com
		putlocker_com.cpp			putlocker.com
		gamefront_com.cpp			gamefront.com
		easybytez_com.cpp			easybytez.com
		remixshare_com.cpp			remixshare.com
		yourfiles_to.cpp			yourfiles.to
)

project(DownloadDaemon-Plugins)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckTypeSize)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckCXXSourceCompiles)

INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/../../CheckCxx11.cmake)

#SET(DD_CONF_DIR "\"/etc/downloaddaemon/\"")
#SET(${DD_CONF_DIR}_OPTION "ON")

#CHECK_INCLUDE_FILE_CXX (syslog.h HAVE_SYSLOG_H)
CHECK_CXX_COMPILER_FLAG(-std=c++0x HAVE_STD_CXX0X)

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
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_FLAG}")
		#SET_TARGET_PROPERTIES(${plugins} PROPERTIES COMPILE_FLAGS ${CXX11_FLAG})
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

	install(CODE "EXEC_PROGRAM(\"tar xjf \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/netload.in.tar.bz2 -C \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/downloaddaemon/plugins/captchadb/ \")")


ENDIF(CURL_FOUND AND HAVE_POPEN)


