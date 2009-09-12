#!/bin/bash
rm -rf ../version

# ddclient-wx...
mkdir -p ../version/ddclient-wx-${1}/src/ddclient-wx
mkdir -p ../version/ddclient-wx-${1}/src/lib
cp -rf ../src/ddclient-wx ../version/ddclient-wx-${1}/src/
cp -rf ../src/lib/netpptk ../version/ddclient-wx-${1}/src/lib
cp -rf ../share ../version/ddclient-wx-${1}
echo "cmake_minimum_required (VERSION 2.6)

project(ddclient-wx)
add_subdirectory(src)" > ../version/ddclient-wx-${1}/CMakeLists.txt

echo "cmake_minimum_required (VERSION 2.6)

project(ddclient-wx)
add_subdirectory(ddclient-wx)
add_subdirectory(lib)" > ../version/ddclient-wx-${1}/src/CMakeLists.txt









cd ../version
find -name .svn | xargs rm -rf
find -name "*~" | xargs rm -f



