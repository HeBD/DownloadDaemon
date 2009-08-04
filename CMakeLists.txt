cmake_minimum_required (VERSION 2.6)
set(CMAKE_VERBOSE_MAKEFILE true)
set(CMAKE_BUILD_TYPE Debug)

project (DownloadDaemon)
add_subdirectory (src/daemon)
add_subdirectory (src/lib)

project (plugins)
add_subdirectory (src/daemon/plugins)

project (ddclient)
add_subdirectory (src/ddclient)
add_subdirectory (src/lib)

project (ddclient-wx)
add_subdirectory (src/ddclient-wx)
add_subdirectory (src/lib)



