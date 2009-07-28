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
		if(it->status == DOWNLOAD_RUNNING) {
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
		dlfile << it->serialize() << '\n';
	}
	dlfile.close();
	return true;
}
