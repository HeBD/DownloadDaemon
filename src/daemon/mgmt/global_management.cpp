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

std::mutex once_per_sec_mutex;

void do_once_per_second() {
	while(true) {
		once_per_sec_mutex.lock();
		global_download_list.purge_deleted();
		if(global_download_list.total_downloads() > 0) {
			global_download_list.decrease_waits();
			dlindex downloadable = global_download_list.get_next_downloadable();
			if(downloadable.first != LIST_ID) {
				global_download_list.do_download(downloadable);
			}
		}
	}
}
