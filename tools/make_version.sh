#!/bin/bash

if [ "$1" == "" ]
then
	echo "No version number specified"
	exit
fi

rm -rf ../version/*-${1}*

# ddclient-wx...
echo "copying ddclient-wx..."
mkdir -p ../version/ddclient-wx-${1}/src/
mkdir -p ../version/ddclient-wx-${1}/src/lib
mkdir -p ../version/ddclient-wx-${1}/share
cp -rf ../src/ddclient-wx ../version/ddclient-wx-${1}/src/
cp -rf ../src/lib/netpptk ../version/ddclient-wx-${1}/src/lib
cp -rf ../share/applications ../share/ddclient-wx ../share/doc ../share/pixmaps ../version/ddclient-wx-${1}/share/
cp -f ../AUTHORS ../CHANGES ../TODO ../LICENCE ../INSTALLING ../version/ddclient-wx-${1}/
echo "cmake_minimum_required (VERSION 2.6)

project(ddclient-wx)
add_subdirectory(src)" > ../version/ddclient-wx-${1}/CMakeLists.txt

echo "cmake_minimum_required (VERSION 2.6)

project(ddclient-wx)
add_subdirectory(ddclient-wx)
add_subdirectory(lib)" > ../version/ddclient-wx-${1}/src/CMakeLists.txt



# DownloadDaemon
echo "copying DownloadDaemon"
mkdir -p ../version/downloaddaemon-${1}/src/ ../version/downloaddaemon-${1}/share/ ../version/downloaddaemon-${1}/etc
cp -rf ../src/daemon ../version/downloaddaemon-${1}/src/
cp -rf ../src/lib ../version/downloaddaemon-${1}/src/
cp -rf ../share/downloaddaemon ../version/downloaddaemon-${1}/share/
cp -rf ../etc/downloaddaemon ../version/downloaddaemon-${1}/etc/downloaddaemon
cp -f ../AUTHORS ../CHANGES ../TODO ../LICENCE ../INSTALLING ../version/downloaddaemon-${1}/
echo "cmake_minimum_required (VERSION 2.6)

project(DownloadDaemon)
add_subdirectory(src)" > ../version/downloaddaemon-${1}/CMakeLists.txt

echo "cmake_minimum_required (VERSION 2.6)

project(DownloadDaemon)
add_subdirectory(daemon)" > ../version/downloaddaemon-${1}/src/CMakeLists.txt


# ddconsole
echo "copying ddconsole"
mkdir -p ../version/ddconsole-${1}/src/
mkdir -p ../version/ddconsole-${1}/src/lib
cp -rf ../src/ddconsole ../version/ddconsole-${1}/src
cp -rf ../src/lib/netpptk ../version/ddconsole-${1}/src/lib/
cp -f ../AUTHORS ../CHANGES ../TODO ../LICENCE ../INSTALLING ../version/ddconsole-${1}/
echo "cmake_minimum_required (VERSION 2.6)

project(ddconsole)
add_subdirectory(src)" > ../version/ddconsole-${1}/CMakeLists.txt

echo "cmake_minimum_required (VERSION 2.6)

project(ddconsole)
add_subdirectory(ddconsole)" > ../version/ddconsole-${1}/src/CMakeLists.txt


# ddclient-php
echo "copying ddclient-php"
mkdir -p ../version/ddclient-php-${1}
cp -rf ../src/ddclient-php/* ../version/ddclient-php-${1}
cp -f ../AUTHORS ../CHANGES ../TODO ../LICENCE ../INSTALLING ../version/ddclient-php-${1}/



echo "removing unneeded files..."
cd ../version
find -name .svn | xargs rm -rf
find -name "*~" | xargs rm -f

echo "building source archives"
tar -c downloaddaemon-${1} > downloaddaemon-${1}.tar
tar -c ddclient-wx-${1} > ddclient-wx-${1}.tar
tar -c ddconsole-${1} > ddconsole-${1}.tar
tar -c ddclient-php-${1} > ddclient-php-${1}.tar
echo "bzip2 source archives"
bzip2 downloaddaemon-${1}.tar
bzip2 ddclient-wx-${1}.tar
bzip2 ddconsole-${1}.tar
bzip2 ddclient-php-${1}.tar



