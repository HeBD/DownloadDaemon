#!/bin/bash

if [ "$1" == "" ]
then
	echo "No version number specified"
	exit
fi

rm -rf ../version

# ddclient-wx...
mkdir -p ../version/ddclient-wx-${1}/src/
mkdir -p ../version/ddclient-wx-${1}/src/lib
cp -rf ../src/ddclient-wx ../version/ddclient-wx-${1}/src/
cp -rf ../src/lib/netpptk ../version/ddclient-wx-${1}/src/lib
cp -rf ../share ../version/ddclient-wx-${1}/
cp -f ../AUTHORS ../CHANGES ../TODO ../LICENCE ../INSTALLING ../version/ddclient-wx-${1}/
echo "cmake_minimum_required (VERSION 2.6)

project(ddclient-wx)
add_subdirectory(src)" > ../version/ddclient-wx-${1}/CMakeLists.txt

echo "cmake_minimum_required (VERSION 2.6)

project(ddclient-wx)
add_subdirectory(ddclient-wx)
add_subdirectory(lib)" > ../version/ddclient-wx-${1}/src/CMakeLists.txt



# DownloadDaemon
mkdir -p ../version/downloaddaemon-${1}/src/
cp -rf ../src/daemon ../version/downloaddaemon-${1}/src/
cp -rf ../src/lib ../version/downloaddaemon-${1}/src/
cp -rf ../program_data ../version/downloaddaemon-${1}/
cp -f ../AUTHORS ../CHANGES ../TODO ../LICENCE ../INSTALLING ../version/downloaddaemon-${1}/
echo "cmake_minimum_required (VERSION 2.6)

project(DownloadDaemon)
add_subdirectory(src)" > ../version/downloaddaemon-${1}/CMakeLists.txt

echo "cmake_minimum_required (VERSION 2.6)

project(DownloadDaemon)
add_subdirectory(daemon)" > ../version/downloaddaemon-${1}/src/CMakeLists.txt


# ddclient
mkdir -p ../version/ddclient-${1}/src/
mkdir -p ../version/ddclient-${1}/src/lib
cp -rf ../src/ddclient ../version/ddclient-${1}/src
cp -rf ../src/lib/netpptk ../version/ddclient-${1}/src/lib/
cp -f ../AUTHORS ../CHANGES ../TODO ../LICENCE ../INSTALLING ../version/ddclient-${1}/
echo "cmake_minimum_required (VERSION 2.6)

project(ddclient)
add_subdirectory(src)" > ../version/ddclient-${1}/CMakeLists.txt

echo "cmake_minimum_required (VERSION 2.6)

project(ddclient)
add_subdirectory(ddclient)" > ../version/ddclient-${1}/src/CMakeLists.txt


# ddclient-php
mkdir -p ../version/ddclient-php-${1}
cp -rf ../src/ddclient-php/* ../version/ddclient-php-${1}
cp -f ../AUTHORS ../CHANGES ../TODO ../LICENCE ../INSTALLING ../version/ddclient-php-${1}/




cd ../version
find -name .svn | xargs rm -rf
find -name "*~" | xargs rm -f



