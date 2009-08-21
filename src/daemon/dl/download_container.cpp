#include "download_container.h"
#include "download.h"
#include "../../lib/cfgfile/cfgfile.h"
#include <string>
using namespace std;

extern string program_root;
extern cfgfile global_config;

download_container::download_container(const char* filename) {
	from_file(filename);
}

bool download_container::from_file(const char* filename) {
	boost::mutex::scoped_lock(list_mutex);
	list_file = filename;
	ifstream dlist(filename);
	string line;
	while(getline(dlist, line)) {
		download_list.push_back(download(line));
	}
	if(!dlist.good()) {
		dlist.close();
		return false;
	}
	dlist.close();
	return true;
}

download_container::iterator download_container::get_download_by_id(int id) {
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->id == id) {
			return it;
		}
	}
	return download_list.end();
}

int download_container::running_downloads() {
	boost::mutex::scoped_lock(list_mutex);
	int running_downloads = 0;
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->get_status() == DOWNLOAD_RUNNING) {
			++running_downloads;
		}
	}
	return running_downloads;
}

int download_container::total_downloads() {
	return download_list.size();
}

bool download_container::dump_to_file() {
	if(list_file == "") {
		return false;
	}

	fstream dlfile(list_file.c_str(), ios::trunc | ios::out);
	if(!dlfile.good()) {
		return false;
	}
	boost::mutex::scoped_lock(file_mutex);
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		dlfile << it->serialize();
	}
	dlfile.close();
	return true;
}

bool download_container::push_back(download &dl) {
	boost::mutex::scoped_lock(list_mutex);
	download_list.push_back(dl);
	if(!dump_to_file()) {
		download_list.pop_back();
		return false;
	}
	return true;
}

bool download_container::pop_back() {
	boost::mutex::scoped_lock(list_mutex);
	download tmpdl = download_list.back();
	download_list.pop_back();
	if(!dump_to_file()) {
		download_list.push_back(tmpdl);
		return false;
	}
	return true;
}

bool download_container::erase(download_container::iterator it) {
	if(it == download_list.end()) {
		return false;
	}
	download tmpdl = *it;
	download_list.erase(it);
	if(!dump_to_file()) {
		download_list.push_back(tmpdl);
		return false;
	}
	return true;
}

int download_container::move_up(int id) {
	boost::mutex::scoped_lock(list_mutex);
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.begin() || it == download_list.end()) {
		return -1;
	}

	download_container::iterator it2 = it;
	--it2;
	while(it2->get_status() == DOWNLOAD_DELETED) {
		--it2;
		if(it2 == download_list.begin()) {
			return -1;
		}
	}

	int iddown = it2->id;
	it2->id = it->id;
	it->id = iddown;
	if(!dump_to_file()) {
		it->id = it2->id;
		it2->id = iddown;
		return -2;
	}
	arrange_by_id();
	return 0;
}

int download_container::get_next_id() {
	boost::mutex::scoped_lock(list_mutex);
	int max_id = -1;
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->id > max_id) {
			max_id = it->id;
		}
	}
	return ++max_id;
}

void download_container::arrange_by_id() {
	boost::mutex::scoped_lock(list_mutex);
	sort(download_list.begin(), download_list.end());
}

download_container::iterator download_container::get_next_downloadable() {
	boost::mutex::scoped_lock(list_mutex);
	download_container::iterator downloadable = download_list.end();
	if(download_list.empty() || running_downloads() >= atoi(global_config.get_cfg_value("simultaneous_downloads").c_str())
	   || global_config.get_cfg_value("downloading_active") == "0") {
		return downloadable;
	}

	// Checking if we are in download-time...
	string dl_start(global_config.get_cfg_value("download_timing_start"));
	if(global_config.get_cfg_value("download_timing_start").find(':') != string::npos && !global_config.get_cfg_value("download_timing_end").find(':') != string::npos ) {
		time_t rawtime;
		struct tm * current_time;
		time ( &rawtime );
		current_time = localtime ( &rawtime );
		int starthours, startminutes, endhours, endminutes;
		starthours = atoi(global_config.get_cfg_value("download_timing_start").substr(0, global_config.get_cfg_value("download_timing_start").find(':')).c_str());
		endhours = atoi(global_config.get_cfg_value("download_timing_end").substr(0, global_config.get_cfg_value("download_timing_end").find(':')).c_str());
		startminutes = atoi(global_config.get_cfg_value("download_timing_start").substr(global_config.get_cfg_value("download_timing_start").find(':') + 1).c_str());
		endminutes = atoi(global_config.get_cfg_value("download_timing_end").substr(global_config.get_cfg_value("download_timing_end").find(':') + 1).c_str());
		// we have a 0:00 shift
		if(starthours > endhours) {
			if(current_time->tm_hour < starthours && current_time->tm_hour > endhours) {
				return downloadable;
			}
			if(current_time->tm_hour == starthours) {
				if(current_time->tm_min < startminutes) {
					return downloadable;
				}
			} else if(current_time->tm_hour == endhours) {
				if(current_time->tm_min > endminutes) {
					return downloadable;
				}
			}
		// download window are just a few minutes
		} else if(starthours == endhours) {
			if(current_time->tm_min < startminutes || current_time->tm_min > endminutes) {
				return downloadable;
			}
		// no 0:00 shift
		} else if(starthours < endhours) {
			if(current_time->tm_hour < starthours || current_time->tm_hour > endhours) {
				return downloadable;
			}
			if(current_time->tm_hour == starthours) {
				if(current_time->tm_min < startminutes) {
					return downloadable;
				}
			} else if(current_time->tm_hour == endhours) {
				if(current_time->tm_min > endminutes) {
					return downloadable;
				}
			}
		}
	}


	for(iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->get_status() != DOWNLOAD_INACTIVE && it->get_status() != DOWNLOAD_FINISHED && it->get_status() != DOWNLOAD_RUNNING
		   && it->get_status() != DOWNLOAD_WAITING && it->get_status() != DOWNLOAD_DELETED) {
			string current_host(it->get_host());
			bool can_attach = true;
			hostinfo hinfo = it->get_hostinfo();
			for(download_container::iterator it2 = download_list.begin(); it2 != download_list.end(); ++it2) {
				if(it2->get_host() == current_host && (it2->get_status() == DOWNLOAD_RUNNING || it2->get_status() == DOWNLOAD_WAITING) && !hinfo.allows_multiple_downloads_free) {
					can_attach = false;
				}
			}
			if(can_attach) {
				downloadable = it;
				break;
			}
		}
	}
	return downloadable;
}

