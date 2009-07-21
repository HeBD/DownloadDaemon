#include <fstream>
#include <vector>
#include "../dl/download.h"
using namespace std;

size_t write_file(void *buffer, size_t size, size_t nmemb, void *userp) {
	fstream* output_file = (fstream*)userp;
	output_file->write((char*)buffer, nmemb);
	return nmemb;
}

int report_progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	// careful! this is a POINTER to an iterator!
	vector<download>::iterator *itp = (vector<download>::iterator*)clientp;
	vector<download>::iterator it = *itp;
	it->size = dltotal;
	it->downloaded_bytes = dlnow;
	return 0;
}