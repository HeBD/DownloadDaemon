#!/bin/bash

if [ "$1" == "" ]
then
	echo "No version number specified"
	echo "usage: $0 version email [ubuntu revision]"
	exit
fi

if [ "$2" == "" ]
then
	echo "No email address specified"
	echo "usage: $0 version email [ubuntu revision]"	
	exit
fi
UBUNTU_REV=$3
if [ "$3" == "" ]
then
	echo "No ubuntu revision specified, assuming 1"
	UBUNTU_REV="1"
fi

# debian target distribution
DEB_DIST="jaunty"

# upstream authors "Name LastName <mail@foo.foo>", seperated by newline and 4 spaces
UPSTREAM_AUTHORS="Adrian Batzill <adrian # batzill ! com>
    Susanne Eichel <susanne.eichel # web ! de>"

# copyright holders "Copyright (C) {Year(s)} by {Author(s)} {Email address(es)}", seperated by newline and 4 spaces
COPYRIGHT_HOLDERS="Copyright (C) 2009 by Adrian Batzill <adrian # batzill ! com>
    Copyright (C) 2009 by Susanne Eichel <susanne.eichel # web ! de>"

# build dependencies
BUILDDEP_DD="debhelper (>= 7), cmake, libboost-thread-dev (>=1.37.0) | libboost-thread1.37-dev, libcurl-gnutls-dev"
BUILDDEP_DDCLIENTWX="debhelper (>= 7), cmake, libboost-thread-dev, libwxbase2.8-dev, libwxgtk2.8-dev"
BUILDDEP_DDCONSOLE="debhelper (>= 7), cmake"

# dependencies (leave empty for auto-detection)
DEP_DD=""
DEP_DDCLIENTWX=""
DEP_DDCONSOLE=""

# synopsis (up to 60 characters)
SYN_DD="A remote controlable download manager"
SYN_DDCLIENTWX="A graphical DownloadDaemon client"
SYN_DDCONSOLE="A DownloadDaemon console client"

# description (if you want a new line, you need to put a space right behind it)
DESC_DD="A remote controlable download manager
 DownloadDaemon is a download manager with many features
 including parsing of one click hoster, automatic
 reconnection, etc. In order to control the daemon
 you need a suitable client, which can be optained from
 http://downloaddaemon.sourceforge.net/"
DESC_DDCLIENTWX="With ddclient-wx you can manage your DownloadDaemon
 server in a comfortable way."
DESC_DDCONSOLE="with ddclient you can easily manage your
 DownloadDaemon server - even without X"

rm -rf ../version/*-${1}* ../version/debs_${1}

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

echo "building source archives..."
tar -cz downloaddaemon-${1} > downloaddaemon-${1}.tar.gz
tar -cz ddclient-wx-${1} > ddclient-wx-${1}.tar.gz
tar -cz ddconsole-${1} > ddconsole-${1}.tar.gz
tar -cz ddclient-php-${1} > ddclient-php-${1}.tar.gz


echo "creating debian packages..."
mkdir debs_${1}
cp downloaddaemon-${1}.tar.gz debs_${1}/downloaddaemon_${1}.orig.tar.gz
cp ddclient-wx-${1}.tar.gz debs_${1}/ddclient-wx_${1}.orig.tar.gz
cp ddconsole-${1}.tar.gz debs_${1}/ddconsole_${1}.orig.tar.gz
cd debs_${1}
tar xfz downloaddaemon_${1}.orig.tar.gz
tar xfz ddclient-wx_${1}.orig.tar.gz
tar xfz ddconsole_${1}.orig.tar.gz
cd downloaddaemon-${1}
echo "Settings for DownloadDaemon:"
dh_make -s -c gpl -e $2
cd debian
#rm *.ex *.EX
cd ../../ddclient-wx-${1}
echo "Settings for ddclient-wx:"
dh_make -s -c gpl -e $2
cd debian
#rm *.ex *.EX
cd ../../ddconsole-${1}
echo "Settings for ddconsole:"
dh_make -s -c gpl -e $2
cd debian
#rm *.ex *.EX
cd ../..

if [ "$DEP_DD" == "" ]; then
	DEP_DD='${shlibs:Depends}, ${misc:Depends}'
fi

if [ "$DEP_DDCLIENTWX" == "" ]; then
	DEP_DDCLIENTWX='${shlibs:Depends}, ${misc:Depends}'
fi
if [ "$DEP_DDCONSOLE" == "" ]; then
	DEP_DDCONSOLE='${shlibs:Depends}, ${misc:Depends}'
fi

################################################################################
echo "Preparing debian/* files for DownloadDaemon"
cd downloaddaemon-${1}/debian
replace="$(<changelog)"
replace="${replace/${1}-1/${1}-0ubuntu${UBUNTU_REV}}"
replace="${replace/unstable/$DEB_DIST}"
echo "$replace" > changelog

replace=$(<control)
replace="${replace/'Section: unknown'/Section: net}"
replace="${replace/'Homepage: <insert the upstream URL, if relevant>'/Homepage: http://downloaddaemon.sourceforge.net/
Suggests: ddclient-wx, ddconsole}"
replace="${replace/'Description: <insert up to 60 chars description>'/Description: $SYN_DD}"
replace="${replace/'Build-Depends: debhelper (>= 7), cmake'/Build-Depends: $BUILDDEP_DD}"
replace="${replace/'Depends: ${shlibs:Depends}, ${misc:Depends}'/Depends: $DEP_DD}"
replace="${replace/'<insert long description, indented with spaces>'/$DESC_DD}"
echo "$replace" > control

replace="$(<copyright)"
replace="${replace/<url:\/\/example.com>/http://downloaddaemon.sourceforge.net/}"
replace="${replace/<put author\'s name and email here>
    <likewise for another author>/$UPSTREAM_AUTHORS}"
replace="${replace/<Copyright (C) YYYY Firtname Lastname>
    <likewise for another author>/$COPYRIGHT_HOLDERS}"
replace="${replace/
### SELECT: ###/}"
replace="${replace/
### OR ###
   This package is free software; you can redistribute it and\/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.
##########/}"
replace="${replace/
# Please also look if there are files or directories which have a
# different copyright\/license attached and list them here./}"
echo "$replace" > copyright

replace="$(<dirs)"
replace+="
/etc/downloaddaemon
/usr/share"
echo "$replace" > dirs

rm docs cron.d.ex downloaddaemon.default.ex downloaddaemon.doc-base.EX emacsen-install.ex emacsen-remove.ex emacsen-startup.ex init.d.ex init.d.lsb.ex manpage.* menu.ex README.Debian watch.ex postinst.ex  postrm.ex  preinst.ex  prerm.ex
cd ../..

########################################################################
echo "Preparing debian/* files for ddclient-wx"
cd ddclient-wx-${1}/debian
replace="$(<changelog)"
replace="${replace/${1}-1/${1}-0ubuntu${UBUNTU_REV}}"
replace="${replace/unstable/$DEB_DIST}"
echo "$replace" > changelog

replace=$(<control)
replace="${replace/'Section: unknown'/Section: net}"
replace="${replace/'Homepage: <insert the upstream URL, if relevant>'/Homepage: http://downloaddaemon.sourceforge.net/
Suggests: downloaddaemon, ddconsole}"
replace="${replace/'Description: <insert up to 60 chars description>'/Description: $SYN_DDCLIENTWX}"
replace="${replace/'Build-Depends: debhelper (>= 7), cmake'/Build-Depends: $BUILDDEP_DDCLIENTWX}"
replace="${replace/'Depends: ${shlibs:Depends}, ${misc:Depends}'/Depends: $DEP_DDCLIENTWX}"
replace="${replace/'<insert long description, indented with spaces>'/$DESC_DDCLIENTWX}"
echo "$replace" > control

replace="$(<copyright)"
replace="${replace/<url:\/\/example.com>/http://downloaddaemon.sourceforge.net/}"
replace="${replace/<put author\'s name and email here>
    <likewise for another author>/$UPSTREAM_AUTHORS}"
replace="${replace/<Copyright (C) YYYY Firtname Lastname>
    <likewise for another author>/$COPYRIGHT_HOLDERS}"
replace="${replace/
### SELECT: ###/}"
replace="${replace/
### OR ###
   This package is free software; you can redistribute it and\/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.
##########/}"
replace="${replace/
# Please also look if there are files or directories which have a
# different copyright\/license attached and list them here./}"
echo "$replace" > copyright

replace="$(<dirs)"
replace+="
/usr/share"
echo "$replace" > dirs

rm docs cron.d.ex ddclient-wx.default.ex ddclient-wx.doc-base.EX emacsen-install.ex emacsen-remove.ex emacsen-startup.ex init.d.ex init.d.lsb.ex manpage.* menu.ex README.Debian watch.ex postinst.ex  postrm.ex  preinst.ex  prerm.ex
cd ../..
######################################################################
echo "Preparing debian/* files for ddconsole"
cd ddconsole-${1}/debian
replace="$(<changelog)"
replace="${replace/${1}-1/${1}-0ubuntu${UBUNTU_REV}}"
replace="${replace/unstable/$DEB_DIST}"
echo "$replace" > changelog

replace=$(<control)
replace="${replace/'Section: unknown'/Section: net}"
replace="${replace/'Homepage: <insert the upstream URL, if relevant>'/Homepage: http://downloaddaemon.sourceforge.net/
Suggests: downloaddaemon, ddclient-wx}"
replace="${replace/'Description: <insert up to 60 chars description>'/Description: $SYN_DDCONSOLE}"
replace="${replace/'Build-Depends: debhelper (>= 7), cmake'/Build-Depends: $BUILDDEP_DDCONSOLE}"
replace="${replace/'Depends: ${shlibs:Depends}, ${misc:Depends}'/Depends: $DEP_DDCONSOLE}"
replace="${replace/'<insert long description, indented with spaces>'/$DESC_DDCONSOLE}"
echo "$replace" > control

replace="$(<copyright)"
replace="${replace/<url:\/\/example.com>/http://downloaddaemon.sourceforge.net/}"
replace="${replace/<put author\'s name and email here>
    <likewise for another author>/$UPSTREAM_AUTHORS}"
replace="${replace/<Copyright (C) YYYY Firtname Lastname>
    <likewise for another author>/$COPYRIGHT_HOLDERS}"
replace="${replace/
### SELECT: ###/}"
replace="${replace/
### OR ###
   This package is free software; you can redistribute it and\/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.
##########/}"
replace="${replace/
# Please also look if there are files or directories which have a
# different copyright\/license attached and list them here./}"
echo "$replace" > copyright

rm docs cron.d.ex ddconsole.default.ex ddconsole.doc-base.EX emacsen-install.ex emacsen-remove.ex emacsen-startup.ex init.d.ex init.d.lsb.ex manpage.* menu.ex README.Debian watch.ex postinst.ex  postrm.ex  preinst.ex  prerm.ex
cd ../..

####################################################################
echo "


"
read -p "Basic debian package preparation done. Please move to <svn root>/version/debs_${1}/*/debian and modify the files as you need them. Then press [enter] to continue package building
expecially the changelog file should be changed."


echo "building downloaddaemon package..."
cd downloaddaemon-${1}
debuild -d -S -sa
cd ..
echo "building ddclient-wx package..."
cd ddclient-wx-${1}
debuild -d -S -sa
cd ..
echo "building ddconsole package..."
cd ddconsole-${1}
debuild -d -S -sa
cd ..
