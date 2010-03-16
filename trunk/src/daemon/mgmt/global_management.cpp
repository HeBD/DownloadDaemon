/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <config.h>
#include <vector>
#include <string>

#include <curl/curl.h>

#include "../dl/download.h"
#include "../dl/download_container.h"
#include "../tools/helperfunctions.h"
#include <cfgfile/cfgfile.h>
#include "global_management.h"
#include "../global.h"

#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

namespace global_mgmt {
	std::mutex ns_mutex;
	std::mutex once_per_sec_mutex;
	std::string curr_start_time;
	std::string curr_end_time;
	bool downloading_active;
}


void do_once_per_second() {
	bool was_in_dltime_last = global_download_list.in_dl_time_and_dl_active();

	while(true) {
		global_mgmt::once_per_sec_mutex.lock();
		global_download_list.purge_deleted();
		if(global_download_list.total_downloads() > 0) {
			global_download_list.decrease_waits();
		}

		bool in_dl_time = global_download_list.in_dl_time_and_dl_active();
		if(!was_in_dltime_last && in_dl_time) {
			// we just reached dl_start time
			global_download_list.start_next_downloadable();
			was_in_dltime_last = true;
		}

		if(was_in_dltime_last && !in_dl_time) {
			// we just left dl_end time
			was_in_dltime_last = false;
		}
	}
}
