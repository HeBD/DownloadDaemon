#include "plugin_helpers.h"
#include <curl/curl.h>
#include <cstdlib>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	mt_string *blubb = (mt_string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	CURL* prepare_handle = curl_easy_init();

	mt_string resultstr;
	curl_easy_setopt(prepare_handle, CURLOPT_URL, get_url());
	curl_easy_setopt(prepare_handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(prepare_handle, CURLOPT_WRITEDATA, &resultstr);
	int success = curl_easy_perform(prepare_handle);

	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	if(resultstr.find("The file could not be found") != mt_string::npos || resultstr.find("404 Not Found") != mt_string::npos || resultstr.find("file has been removed") != mt_string::npos) {
		return PLUGIN_FILE_NOT_FOUND;
	}

	unsigned int pos;
	if((pos = resultstr.find("<form id=\"ff\"")) == mt_string::npos) {
		return PLUGIN_ERROR;
	}

	mt_string url;
	pos = resultstr.find("http://", pos);
	unsigned int end = resultstr.find('\"', pos);

	url = resultstr.substr(pos, end - pos);

	resultstr.clear();
	curl_easy_setopt(prepare_handle, CURLOPT_LOW_SPEED_LIMIT, 100);
	curl_easy_setopt(prepare_handle, CURLOPT_LOW_SPEED_TIME, 20);
	curl_easy_setopt(prepare_handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(prepare_handle, CURLOPT_POST, 1);
	curl_easy_setopt(prepare_handle, CURLOPT_POSTFIELDS, "dl.start=\"Free\"");
	curl_easy_perform(prepare_handle);

	if(resultstr.find("Or try again in about") != mt_string::npos) {
		pos = resultstr.find("Or try again in about");
		pos = resultstr.find("about", pos);
		pos = resultstr.find(' ', pos);
		++pos;
		end = resultstr.find(' ', pos);
		mt_string have_to_wait(resultstr.substr(pos, end - pos));
		set_wait_time(atoi(have_to_wait.c_str()) * 60);
		return PLUGIN_LIMIT_REACHED;
	} else if(resultstr.find("Please try again in 2 minutes") != mt_string::npos) {
		set_wait_time(120);
		return PLUGIN_SERVER_OVERLOADED;
	} else {
		if((pos = resultstr.find("var c")) == mt_string::npos) {
			// Unable to get the wait time, will wait 135 seconds
			set_wait_time(135);
		} else {
			pos = resultstr.find("=", pos) + 1;
			end = resultstr.find(";", pos) - 1;
			set_wait_time(atoi(resultstr.substr(pos, end).c_str()));
		}

		if((pos = resultstr.find("<form name=\"dlf\"")) == mt_string::npos) {
			return PLUGIN_ERROR;
		}

		pos = resultstr.find("http://", pos);
		end = resultstr.find('\"', pos);

		outp.download_url = resultstr.substr(pos, end - pos);
		return PLUGIN_SUCCESS;
	}
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		outp.allows_resumption = true;
		outp.allows_multiple = true;
	} else {
		outp.allows_resumption = false;
		outp.allows_multiple = false;
	}
}
