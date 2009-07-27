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

#include <sys/stat.h>
#include <sys/types.h>

using namespace std;


extern cfgfile global_config;

int main(int argc, char* argv[]) {
	if(argc == 2) {
		string argv1 = argv[1];
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

	string root_dir(argv[0]);

	root_dir = root_dir.substr(0, root_dir.find_last_of("/\\"));
	program_root = root_dir;
	program_root += '/';

	struct stat st;
	if(stat(string(root_dir + "/conf/downloaddaemon.conf").c_str(), &st) != 0) {
		log_string("Could not locate configuration file!", LOG_SEVERE);
		exit(-1);
	}
	global_config.open_cfg_file(root_dir + "/conf/downloaddaemon.conf", true);


	ifstream dlist(string(program_root + global_config.get_cfg_value("dlist_file")).c_str());
	string line;
	while(getline(dlist, line)) {
		download dl(line);
		global_download_list.push_back(dl);
	}
	dlist.close();

	log_string("DownloadDaemon started successfully", LOG_DEBUG);

	boost::thread mgmt_thread(mgmt_thread_main);
	boost::thread download_thread(download_thread_main);
	boost::thread download_ticker(tick_downloads);

	download_ticker.join();
	download_thread.join();
	mgmt_thread.join();
}
