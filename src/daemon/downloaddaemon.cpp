#include "../lib/cfgfile.h"
#include "mgmt/mgmt_thread.h"
#include "dl/download.h"
#include "dl/download_thread.h"
#include "global.h"
#include "mgmt/global_management.h"
#include "tools/helperfunctions.h"
#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <vector>
#include <string>

using namespace std;


extern cfgfile global_config;

int main(int argc, char* argv[]) {
	string root_dir(argv[0]);

	root_dir = root_dir.substr(0, root_dir.find_last_of("/\\"));
	program_root = root_dir;
	program_root += '/';
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
