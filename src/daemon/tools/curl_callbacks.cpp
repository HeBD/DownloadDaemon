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
	int id = *(int*)clientp;
	global_download_list.set_int_property(id, DL_SIZE, dltotal);
	mt_string output_file;
	try {
		output_file = global_download_list.get_string_property(id, DL_OUTPUT_FILE);
	} catch(download_exception &e) {
		log_string("Download ID " + int_to_string(id) + " has been deleted internally while actively downloading. Please report this bug!", LOG_SEVERE);
		global_download_list.deactivate(id);
		return 0;
	}

	struct stat st;
	if(stat(output_file.c_str(), &st) == 0) {
	    global_download_list.set_int_property(id, DL_DOWNLOADED_BYTES, st.st_size);
	} else {
		global_download_list.set_int_property(id, DL_DOWNLOADED_BYTES, dlnow);
	}
	return 0;
}
