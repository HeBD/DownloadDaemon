#!/bin/bash


# set this variable to the svn-trunk directory to manage
trunk_root="/home/adrian/downloaddaemon/trunk"

# set this to the e-mail adress to use for signing the packages
sign_email="agib@gmx.de" # "example@example.com"

# ubuntu ppa to upload to
ppa="ppa:agib/ppa"


set -x

function log () {
	echo "[`date`]: $1" >> $trunk_root/tools/autobuild.log
}

cd "${trunk_root}/tools"
svn up
cd "$trunk_root/src/daemon"
if [ `svn up | wc -l` -le 1 ]; then
	log "no updates available to build"
	exit 0;
fi


cd "$trunk_root/tools"
version="`svn info | grep Revision | cut -f2 -d ' ' /dev/stdin`"

./make_daemon.sh $version $sign_email 1 nightly

cd ../version/${version}/debs_${version}

for f in $( ls ); do
	dput $ppa ${f}/downloaddaemon-nightly_${version}-0ubuntu1+agib~${f}1_source.changes
done

cd ..
cp downloaddaemon-nightly-${version}.tar.gz ${trunk_root}/../tags

cd ${trunk_root}/..
svn up
cd tags
svn add downloaddaemon-nightly-${version}.tar.gz
svn ci -m "release of DownloadDaemon nightly revision ${version}"
cd ${trunk_root}/version
rm -r ${version}
