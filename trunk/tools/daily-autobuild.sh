#!/bin/bash
# takes only one argument: the passphrase for your private key to sign the files
if [ "$1" == "" ]; then
	echo "Usage: $0 <passphrase>"
	exit -1
fi

# set this variable to the svn-trunk directory to manage
trunk_root="/home/adrian/downloaddaemon/trunk"

# set this to the e-mail adress to use for signing the packages
sign_email="agib@gmx.de" # "example@example.com"

# ubuntu ppa to upload to
ppa="ppa:agib/ppa"


# takes only one argument: the passphrase for your private key to sign the files
if [ "$1" == "" ]; then
	echo "Usage: $0 <passphrase>"
	exit -1
fi

set -x

function log () {
	echo "[`date`]: $1" >> $trunk_root/tools/autobuild.log
}

dd_up=false
gui_up=false
con_up=false
php_up=false

cd "$trunk_root/src/daemon"
if [ `svn up | wc -l` -gt 1 ]; then
	dd_up=true
	log "New DownloadDaemon version available"
fi

cd "$trunk_root/src/ddclient-gui"
if [ `svn up | wc -l` -gt 1 ]; then
	gui_up=true
	log "New ddclient-gui version available"
fi

cd "$trunk_root/src/ddconsole"
if [ `svn up | wc -l` -gt 1 ]; then
	con_up=true
	log "New ddconsole version available"
fi

cd "$trunk_root/src/ddclient-php"
if [ `svn up | wc -l` -gt 1 ]; then
	dd_up=true
	log "New ddclient-php version available"
fi

if [ $dd_up == false -a $gui_up == false -a $con_up == false -a $php_up == false]; then
	log "No updates available tonight"
	exit 0
fi

cd "$trunk_root/.."
svn up
cd "$trunk_root/tools"

version="`svn info | grep Revision | cut -f2 -d ' ' /dev/stdin`"

<<<<<<< .mine
if [ $dd_up ]; then 
	cd "$trunk_root/tools"
	expect -c "
set timeout 120
match_max 100000
spawn ./make_daemon.sh $version $sign_email 1 nightly
expect {
	\"*assphrase:\" {
		send \"$1\r\"
		exp_continue
	}
	eof {
		exit
	}	
}
"
	cd ../version/${version}/debs_${version}
=======
expect -c "
set timeout 120
match_max 100000
spawn ./make_daemon.sh $version $sign_email 1 nightly
expect {
	\"*assphrase:\" {
		send \"$1\r\"
		exp_continue
	}
	eof {
		exit
	}	
}
"
>>>>>>> .r917

	for f in $( ls ); do
		dput $ppa ${f}/downloaddaemon-nightly_${version}-0ubuntu1+agib~${f}1_source.changes
	done

	cd ..
	cp downloaddaemon-nightly-${version}.tar.gz ${trunk_root}/../tags
	svn add ${trunk_root}/../tags/downloaddaemon-nightly-${version}.tar.gz
	cd ${trunk_root}/version
	rm -r ${version}
fi

cd $trunk_root/../tags
svn ci -m "release of DownloadDaemon nightly revision ${version}"

