#include "captcha.h"

#include <string>
#include <iostream>
#include <fstream>
using namespace std;

std::string captcha::process_image(std::string gocr_options, std::string img_type, bool use_db, bool keep_whitespaces) {
	captcha_exception e;
	if(retry_count > max_retrys) {
		throw e;
	}
	if(gocr.empty()) {
		throw e;
	}

	++retry_count;
	string img_fn = "/tmp/captcha_" + host + "." + img_type;
	ofstream ofs(img_fn.c_str());
	ofs << image;
	ofs.close();
	string to_exec = gocr + " " + gocr_options;
	if(use_db) {
		to_exec += " -p plugins/captchadb/" + host + "/";
	}
	to_exec += " " + img_fn + " 2> /dev/null";
	FILE* cap_result = popen(to_exec.c_str(), "r");
	if(cap_result == NULL) {
		captcha_exception e;
		throw e;
	}

	char cap_res_string[256];
	fgets(cap_res_string, 256, cap_result);
	pclose(cap_result);

	remove(img_fn.c_str());

	string final = cap_res_string;
	if(!keep_whitespaces) {
		for(size_t i = 0; i < final.size(); ++i) {
			if(isspace(final[i])) {
				final.erase(i);
				--i;
			}
		}
	}
	return final;
}
