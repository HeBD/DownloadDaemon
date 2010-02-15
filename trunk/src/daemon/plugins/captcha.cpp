/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "captcha.h"

#include <string>
#include <fstream>
#include <cstring>
#include <iostream>
using namespace std;

std::string captcha::process_image(std::string gocr_options, std::string img_type, int required_chars, bool use_db, bool keep_whitespaces) {
	captcha_exception e;
	if(retry_count > max_retrys) {
		throw e;
	}
	struct stat st;
	if(gocr.empty() || stat(gocr.c_str(), &st) != 0) {
		throw e;
	}

	string img_fn = "/tmp/captcha_" + host + "." + img_type;
	ofstream ofs(img_fn.c_str());
	ofs << image;
	ofs.close();
	string to_exec = gocr + " " + gocr_options;
	if(use_db) {
		to_exec += " -p " + share_directory + "/plugins/captchadb/" + host;
        #ifdef __CYGWIN__
        to_exec += "\\\\";
        #else
        to_exec += "/";
        #endif
	}
	to_exec += " " + img_fn + " 2> /dev/null";
	FILE* cap_result = popen(to_exec.c_str(), "r");
	if(cap_result == NULL) {
		captcha_exception e;
		throw e;
	}

	char cap_res_string[256];
	memset(cap_res_string, 0, 256);
	fgets(cap_res_string, 256, cap_result);

	pclose(cap_result);
	//remove(img_fn.c_str());

	string final = cap_res_string;
	if(!keep_whitespaces) {
		for(size_t i = 0; i < final.size(); ++i) {
			if(isspace(final[i])) {
				final.erase(i, 1);
				--i;
			}
		}
	}

	if(required_chars == -1) {
		++retry_count;
	} else {
		final = final.substr(0, required_chars);
		if(final.size() == (size_t)required_chars) {
			++retry_count;
		} else {
			final = "";
		}

	}

	return final;
}
