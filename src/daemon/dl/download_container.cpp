#include "download_container.h"
#include "download.h"
#include <string>
using namespace std;

extern string program_root;

download_container::download_container(const char* filename) {
	from_file(filename);
}

bool download_container::from_file(const char* filename) {
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

	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		dlfile << it->serialize();
	}
	dlfile.close();
	return true;
}

bool download_container::push_back(download &dl) {
	download_list.push_back(dl);
	if(!dump_to_file()) {
		download_list.pop_back();
		return false;
	}
	return true;
}

bool download_container::pop_back() {
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
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.begin() || it == download_list.end()) {
		return -1;
	}
	download_container::iterator it2 = get_download_by_id(id);
	--it2;

	int iddown = it2->id;
	it2->id = it->id;
	it->id = iddown;
	if(!dump_to_file()) {
		it->id = it2->id;
		it2->id = iddown;
		return -2;
	}
	return 0;
}

int download_container::get_next_id() {
	int max_id = -1;
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->id > max_id) {
			max_id = it->id;
		}
	}
	return ++max_id;
}
