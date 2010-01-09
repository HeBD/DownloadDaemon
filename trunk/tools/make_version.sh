#!/bin/bash

# debian target distribution
DEB_DIST="karmic"

# upstream authors "Name LastName <mail@foo.foo>", seperated by newline and 4 spaces
UPSTREAM_AUTHORS="Adrian Batzill <adrian # batzill ! com>
    Susanne Eichel <susanne.eichel # web ! de>"

# copyright holders "Copyright (C) {Year(s)} by {Author(s)} {Email address(es)}", seperated by newline and 4 spaces
COPYRIGHT_HOLDERS="Copyright (C) 2009 by Adrian Batzill <adrian # batzill ! com>
    Copyright (C) 2009 by Susanne Eichel <susanne.eichel # web ! de>"

# build dependencies
BUILDDEP_DD="debhelper (>= 7), cmake, libboost-thread-dev (>=1.37.0) | libboost-thread1.37-dev, libcurl4-gnutls-dev"
BUILDDEP_DDCLIENTWX="debhelper (>= 7), cmake, libboost-thread-dev (>=1.37.0) | libboost-thread1.37-dev, libwxbase2.8-dev, libwxgtk2.8-dev"
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

# specify all files/directorys (array) and the path's where they should go to (basically a cp -r FILES_XXX[i] PATHS_XXX[i] is done)
# the .svn folders are removed automatically. Folders are created automatically before copying
FILES_DD=("../src/daemon" "../src/lib/" "../etc/downloaddaemon" "../etc/init.d/downloadd" "../AUTHORS" "../CHANGES" "../TODO" "../LICENCE" "../INSTALLING")
PATHS_DD=("src/" "src/" "etc/" "/etc/init.d")

FILES_DDCLIENTWX=("../src/ddclient-wx" "../src/lib/netpptk" "../src/lib/crypt" "../share/applications" "../share/ddclient-wx" "../share/doc" "../share/pixmaps" "../AUTHORS" "../CHANGES" "../TODO" "../LICENCE" "../INSTALLING")
PATHS_DDCLIENTWX=("src/" "src/lib/" "src/lib/" "share/" "share/" "share/" "share/" )

FILES_DDCONSOLE=("../src/ddconsole" "../src/lib/netpptk" "../src/lib/crypt" "../AUTHORS" "../CHANGES" "../TODO" "../LICENCE" "../INSTALLING")
PATHS_DDCONSOLE=("src/" "src/lib/" "src/lib/")

FILES_DDCLIENTPHP=("src/ddclient-php/*" "../AUTHORS" "../CHANGES" "../TODO" "../LICENCE" "../INSTALLING")
PATHS_DDCLIENTPHP=("ddclient-php")


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

VERSION=$1
EMAIL=$2


mkdir -p ../version/${VERSION}


for path in $(seq 0 $((${#FILES_DD[@]} - 1))); do
	mkdir -p ../version/${VERSION}/downloaddaemon-${VERSION}/${PATHS_DD[${path}]}
	cp -r ${FILES_DD[${path}]} ../version/${VERSION}/downloaddaemon-${VERSION}/${PATHS_DD[${path}]}
done

for path in $(seq 0 $((${#FILES_DDCLIENTWX[@]} - 1))); do
	mkdir -p ../version/${VERSION}/ddclient-wx-${VERSION}/${PATHS_DDCLIENTWX[${path}]}
	cp -r ${FILES_DDCLIENTWX[${path}]} ../version/${VERSION}/ddclient-wx-${VERSION}/${PATHS_DDCLIENTWX[${path}]}
done

for path in $(seq 0 $((${#FILES_DDCONSOLE[@]} - 1))); do
	mkdir -p ../version/${VERSION}/ddconsole-${VERSION}/${PATHS_DDCONSOLE[${path}]}
	cp -r ${FILES_DDCONSOLE[${path}]} ../version/${VERSION}/ddconsole-${VERSION}/${PATHS_DDCONSOLE[${path}]}
done

for path in $(seq 0 $((${#FILES_DDCLIENTPHP[@]} - 1))); do
	mkdir -p ../version/${VERSION}/ddclient-php-${VERSION}/${PATHS_DDCLIENTPHP[${path}]}
	cp -r ${FILES_DDCLIENTPHP[${path}]} ../version/${VERSION}/ddclient-php-${VERSION}/${PATHS_DDCLIENTPHP[${path}]}
done

cd ../version/${VERSION}

echo "cmake_minimum_required (VERSION 2.6)
project(DownloadDaemon)
add_subdirectory(src/daemon)" > downloaddaemon-${VERSION}/CMakeLists.txt

echo "cmake_minimum_required (VERSION 2.6)
project(ddclient-wx)
add_subdirectory(src/ddclient-wx)" > ddclient-wx-${VERSION}/CMakeLists.txt

echo "cmake_minimum_required (VERSION 2.6)
project(ddconsole)
add_subdirectory(src/ddconsole)" > ddconsole-${VERSION}/CMakeLists.txt

echo "removing unneeded files..."
find -name .svn | xargs rm -rf
find -name "*~" | xargs rm -f


echo "BUILDING SOURCE ARCHIVES..."
tar -cz downloaddaemon-${VERSION} > downloaddaemon-${1}.tar.gz
tar -cz ddclient-wx-${VERSION} > ddclient-wx-${1}.tar.gz
tar -cz ddconsole-${VERSION} > ddconsole-${1}.tar.gz
tar -cz ddclient-php-${VERSION} > ddclient-php-${1}.tar.gz
echo "DONE"
echo "PREPARING DEBIAN ARCHIVES..."
mkdir debs_${VERSION}
# only if it's the first ubuntu rev, we copy the .orig files. otherwise we just update.
if [ "$UBUNTU_REV" == "1" ]; then
	cp downloaddaemon-${VERSION}.tar.gz debs_${VERSION}/downloaddaemon_${VERSION}.orig.tar.gz
	cp ddclient-wx-${VERSION}.tar.gz debs_${VERSION}/ddclient-wx_${VERSION}.orig.tar.gz
	cp ddconsole-${VERSION}.tar.gz debs_${VERSION}/ddconsole_${VERSION}.orig.tar.gz
fi

cd debs_${VERSION}
rm -rf downloaddaemon-${VERSION} ddclient-wx-${VERSION} ddconsole-${VERSION}
cp -rf ../downloaddaemon-${VERSION} ../ddclient-wx-${VERSION} ../ddconsole-${VERSION} .

cd downloaddaemon-${VERSION}
echo "Settings for DownloadDaemon:"
dh_make -s -c gpl -e ${EMAIL}
cd debian
#rm *.ex *.EX
cd ../../ddclient-wx-${VERSION}
echo "Settings for ddclient-wx:"
dh_make -s -c gpl -e ${EMAIL}
cd debian
#rm *.ex *.EX
cd ../../ddconsole-${VERSION}
echo "Settings for ddconsole:"
dh_make -s -c gpl -e ${EMAIL}
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


################################################################################ DOWNLOADDAEMON PREPARATION
echo "Preparing debian/* files for DownloadDaemon"
cd downloaddaemon-${VERSION}/debian
replace="$(<changelog)"
replace="${replace/${VERSION}-1/${VERSION}-0ubuntu${UBUNTU_REV}}"
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

mv postinst.ex postinst
mv postrm.ex postrm
replace="$(<postinst)"
replace="${replace/'    configure)'/    configure)
	if ! getent group downloadd >/dev/null; then
        	addgroup --system downloadd
	fi

	if ! getent passwd downloadd >/dev/null; then
        	adduser --system --ingroup downloadd --home /etc/downloaddaemon downloadd
        	usermod -c DownloadDaemon downloadd
	fi

	if [ -d /etc/downloaddaemon ]; then
  		chown -R downloadd:downloadd /etc/downloaddaemon
	fi
	if [ -x /etc/init.d/downloadd ]; then
		update-rc.d downloadd defaults >/dev/null
		/etc/init.d/downloadd start
	fi
}"
echo "$replace" > postinst

replace="$(<postrm)"
replace="${replace/'    purge|remove|upgrade|failed-upgrade|abort-install|abort-upgrade|disappear)'/    purge|remove|upgrade|failed-upgrade|abort-install|abort-upgrade|disappear)
	if [ \"\$1\" = \"purge\" ] ; then
		update-rc.d downloadd remove >/dev/null || exit \$?
	fi
}"

echo "$replace" > postrm

rm docs cron.d.ex downloaddaemon.default.ex downloaddaemon.doc-base.EX emacsen-install.ex emacsen-remove.ex emacsen-startup.ex init.d.ex init.d.lsb.ex manpage.* menu.ex README.Debian watch.ex  preinst.ex  prerm.ex
cd ../..

######################################################################## DDCLIENT-WX PREPARATION
echo "Preparing debian/* files for ddclient-wx"
cd ddclient-wx-${VERSION}/debian
replace="$(<changelog)"
replace="${replace/${VERSION}-1/${VERSION}-0ubuntu${UBUNTU_REV}}"
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


###################################################################### DDCONSOLE PREPARATION
echo "Preparing debian/* files for ddconsole"
cd ddconsole-${VERSION}/debian
replace="$(<changelog)"
replace="${replace/${VERSION}-1/${VERSION}-0ubuntu${UBUNTU_REV}}"
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
cd downloaddaemon-${VERSION}
debuild -S -sa
cd ..
echo "building ddclient-wx package..."
cd ddclient-wx-${VERSION}
debuild -S -sa
cd ..
echo "building ddconsole package..."
cd ddconsole-${VERSION}
debuild -S -sa
cd ..