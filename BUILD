To build DownloadDaemon, you need to have libcurl (libcurl-dev) and boost_thread installed.
The easyest way to build all programs (server and client) is by changing to the build directory and running
"cmake ..", then "make" and "make install"(as root).

If you only want to build parts of the software, also change to the build directory and run
"cmake ../src/daemon",
"cmake ../src/plugins",
"cmake ../src/ddclient",
"cmake ../src/ddclient-wx"
respectively, followed by "make" and "make install"(as root).
The clients will  be installed to /usr/local/bin. The daemon and it's files will be installed to /opt/downloaddaemon, with a start-script in /usr/local/bin.

All programs are moveable, so if you don't like /opt/downloaddaemon as a location, simply move it to somewhere else (in this case, you have to change the start-script in /usr/local/bin)
