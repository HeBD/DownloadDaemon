#include <fstream>
#include <vector>

#include "../dl/download.h"
#include "../dl/download_container.h"
#include "helperfunctions.h"

#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

extern download_container global_download_list;

size_t write_file(void *buffer, size_t size, size_t nmemb, void *userp) {
	fstream* output_file = (fstream*)userp;
	output_file->write((char*)buffer, nmemb);
	return nmemb;
}

int report_progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	// careful! this is a POINTER to an iterator!
	download_container::iterator *itp = (download_container::iterator*)clientp;
	download_container::iterator it = *itp;
	it->set_size(dltotal);
	struct stat st;
	if(stat(it->get_output_file().c_str(), &st) == 0) {
	    it->set_downloaded_bytes(st.st_size);
	} else {
	    it->set_downloaded_bytes(dlnow);
	}
	global_download_list.dump_to_file();
	return 0;
}
