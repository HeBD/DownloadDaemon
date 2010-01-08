/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <vector>
#include <string>

#include <curl/curl.h>

#include "../dl/download.h"
#include "../dl/download_container.h"
#include "../dl/download_thread.h"
#include "../tools/helperfunctions.h"
#include "../../lib/cfgfile/cfgfile.h"
#include "global_management.h"
#include "../global.h"

#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

void do_once_per_second() {
	if(global_download_list.total_downloads() > 0) {
		global_download_list.decrease_waits();
		global_download_list.purge_deleted();
		start_next_download();
	}
}
