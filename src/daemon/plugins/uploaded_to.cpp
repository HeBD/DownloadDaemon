#include "plugin_helpers.h"
#include <curl/curl.h>
#include <cstdlib>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	string *blubb = (string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

void trim_string(std::string &str) {
	while(str.length() > 0 && isspace(str[0])) {
		str.erase(str.begin());
	}
	while(str.length() > 0 && isspace(*(str.end() - 1))) {
		str.erase(str.end() -1);
	}
}

extern "C" plugin_status plugin_exec(download &dl, CURL* curl_handle, plugin_input &inp, plugin_output &outp) {
	CURL* handle = curl_easy_init();
	string resultstr;
	curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, 100);
	curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, 20);
	curl_easy_setopt(handle, CURLOPT_URL, get_url(dl));
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &resultstr);
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
	int success = curl_easy_perform(handle);

	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	if(resultstr.find("File doesn't exist") != string::npos || resultstr.find("404 Not Found") != string::npos ||
	   resultstr.find("The file status can only be queried by premium users") != string::npos) {
		return PLUGIN_FILE_NOT_FOUND;
	}

	size_t pos;
	if((pos = resultstr.find("You have reached a maximum number of downloads or used up your Free-Traffic!")) != string::npos) {
		pos = resultstr.find("(Or wait ", pos);
		pos += 9;
		size_t end = resultstr.find(' ', pos);
		string wait_time = resultstr.substr(pos, end - pos);
		set_wait_time(dl, atoi(wait_time.c_str()) * 60);
		return PLUGIN_LIMIT_REACHED;
	}

	if((pos = resultstr.find("name=\"download_form\"")) == string::npos || resultstr.find("Filename:") == string::npos) {
		return PLUGIN_ERROR;
	}

	pos = resultstr.find("http://", pos);
	size_t end = resultstr.find('\"', pos);
	outp.download_url = resultstr.substr(pos, end - pos);

	std::string filename;
	pos = resultstr.find("Filename:");
	pos = resultstr.find("<b>", pos) + 3;
	end = resultstr.find("</b>", pos);
	filename = resultstr.substr(pos, end - pos);
	trim_string(filename);
	std::string filetype;
	pos = resultstr.find("Filetype:");
	pos = resultstr.find("<td>", pos) + 4;
	end = resultstr.find("</td>", pos);
	filetype = resultstr.substr(pos, end - pos);
	trim_string(filetype);
	filename += filetype;
	outp.download_filename = filename;
	return PLUGIN_SUCCESS;
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
