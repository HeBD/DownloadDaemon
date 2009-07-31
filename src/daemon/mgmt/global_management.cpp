#include <vector>
#include <curl/curl.h>
#include "../dl/download.h"
#include "../dl/download_container.h"
#include "global_management.h"
using namespace std;

extern download_container global_download_list;

void tick_downloads() {
	while(true) {
		for(download_container::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
			if(it->wait_seconds > 0 && it->get_status() == DOWNLOAD_WAITING) {
				--it->wait_seconds;
				if(it->wait_seconds == 0) {
					it->set_status(DOWNLOAD_PENDING);
				}
			} else if(it->wait_seconds > 0 && it->get_status() == DOWNLOAD_INACTIVE) {
				it->wait_seconds = 0;
			}
		}

		sleep(1);
	}
}
