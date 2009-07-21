INSTALL_PATH=/opt/downloaddaemon

default: all
include src/daemon/Makefile
include src/daemon/plugins/Makefile
include src/ddclient/Makefile

default: all

all: daemon_only plugins ddclient

daemon: daemon_only plugins

daemon_only:
	cd src/daemon; make daemon_only_local

ddclient:
	cd src/ddclient; make ddclient_local

plugins:
	cd src/daemon/plugins; make plugins_local

install:
	-mkdir ${INSTALL_PATH}
	cp -rf built/* "${INSTALL_PATH}"
	echo "#!/bin/bash" > /usr/local/bin/downloaddaemon
	echo "cd ${INSTALL_PATH}" >> /usr/local/bin/downloaddaemon
	echo "./DownloadDaemon \$${@}" >> /usr/local/bin/downloaddaemon
	-mv "${INSTALL_PATH}/ddclient" /usr/local/bin
	chmod +x /usr/local/bin/downloaddaemon

uninstall:
	-rm -f /usr/local/bin/downloaddaemon
	-rm -f /usr/local/bin/ddclient
	-rm -rf /opt/downloaddaemon

clean:
	cd src/daemon; make daemon_only_clean
	cd src/daemon/plugins; make plugins_clean
	cd src/ddclient; make ddclient_clean
