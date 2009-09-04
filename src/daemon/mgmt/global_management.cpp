#include <vector>
#include <string>

#include <curl/curl.h>

#include "../dl/download.h"
#include "../dl/download_container.h"
#include "../dl/download_thread.h"
#include "../tools/helperfunctions.h"
#include "../../lib/cfgfile/cfgfile.h"
#include "global_management.h"

#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

extern download_container global_download_list;
extern cfgfile global_config;
extern cfgfile global_router_config;
extern mt_string program_root;

void tick_downloads() {
	while(true) {
		global_download_list.decrease_waits();
		global_download_list.purge_deleted();
		sleep(1);
	}
}
