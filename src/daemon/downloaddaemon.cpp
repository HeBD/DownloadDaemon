/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "../lib/cfgfile/cfgfile.h"
#include "mgmt/mgmt_thread.h"
#include "dl/download.h"
#include "dl/download_thread.h"
#include "global.h"
#include "mgmt/global_management.h"
#include "tools/helperfunctions.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <climits>
#include <cstdlib>

#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>

using namespace std;


extern cfgfile global_config;

int main(int argc, char* argv[], char* env[]) {
	if(argc == 2) {
		std::string argv1 = argv[1];
		if(argv1 == "-d" || argv1 == "--daemon") {
			int i = fork();
			if (i < 0) return 1; /* fork error */
			if (i > 0) return 0; /* parent exits */
			/* child (daemon) continues */
		} else if(argv1 == "--help") {
			cout << "Usage: downloaddaemon [-d|--daemon]" << endl;
			return 0;
		}
	}

	struct stat st;
	// getting working dir
	char* real_path = realpath(argv[0], 0);
	program_root = real_path;
	free(real_path);
	program_root = program_root.substr(0, program_root.find_last_of('/'));
	program_root = program_root.substr(0, program_root.find_last_of('/'));
	program_root.append("/share/downloaddaemon/");
	if(stat(program_root.c_str(), &st) != 0) {
		cerr << "Unable to locate program data (should be in bindir/../share/downloaddaemon)" << endl;
		exit(-1);
	}
	chdir(program_root.c_str());


	if(stat("/etc/downloaddaemon/downloaddaemon.conf", &st) != 0) {
		cerr << "Could not locate configuration file!" << endl;
		exit(-1);
	}
	if(stat("/etc/downloaddaemon/routerinfo.conf", &st) != 0) {
		cerr << "Could not locate router configuration file!" << endl;
		exit(-1);
	}
	global_config.open_cfg_file("/etc/downloaddaemon/downloaddaemon.conf", true);
	global_router_config.open_cfg_file("/etc/downloaddaemon/routerinfo.conf", true);

	std::string dlist_fn = global_config.get_cfg_value("dlist_file");
	correct_path(dlist_fn);
	global_download_list.from_file(dlist_fn.c_str());

	log_string("DownloadDaemon started successfully", LOG_DEBUG);

	boost::thread mgmt_thread(mgmt_thread_main);
	boost::thread download_thread(download_thread_main);
	boost::thread download_ticker(tick_downloads);
	//boost::thread reconnector(reconnect);

	//reconnector.join();
	download_ticker.join();
	download_thread.join();
	mgmt_thread.join();
}
