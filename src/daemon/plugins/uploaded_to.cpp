#include <curl/curl.h>
#include <string>
#include <cstdlib>
#include <fstream>
#include <iostream>
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

int main(int argc, char* argv[]) {
	if(argc < 3)
		return -1;

	string output_fn(argv[0]);
	output_fn = output_fn.substr(0, output_fn.find_last_of('/'));
	output_fn += "/plugin_comm/";
	output_fn += argv[1];
	fstream output_file(output_fn.c_str(), ios::out);
	CURL* handle = curl_easy_init();
	string resultstr;
	curl_easy_setopt(handle, CURLOPT_URL, argv[2]);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &resultstr);
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
	int success = curl_easy_perform(handle);

	if(success != 0) {
		output_file << "download_parse_success = 0\n";
		output_file << "download_parse_errmsg = CONNECTION_FAILED\n";
		output_file.close();
		return 0;
	}

	if(resultstr.find("File doesn't exist") != string::npos || resultstr.find("404 Not Found") != string::npos ||
	   resultstr.find("The file status can only be queried by premium users") != string::npos) {
		output_file << "download_parse_success = 0\n";
		output_file << "download_parse_errmsg = FILE_NOT_FOUND\n";
		output_file.close();
		return 0;
	}

	size_t pos;
	if((pos = resultstr.find("You have reached a maximum number of downloads or used up your Free-Traffic!")) != string::npos) {
		pos = resultstr.find("(Or wait ", pos);
		pos += 9;
		size_t end = resultstr.find(' ', pos);
		string wait_time = resultstr.substr(pos, end - pos);
		output_file << "download_parse_success = 0\n";
		output_file << "download_parse_errmsg = LIMIT_REACHED\n";
		unsigned int wait_secs = atoi(wait_time.c_str()) * 60;
		output_file << "download_parse_wait = " << wait_secs;
		output_file.close();
		return 0;
	}

	if((pos = resultstr.find("name=\"download_form\"")) == string::npos || resultstr.find("Filename:") == string::npos) {
		output_file << "download_parse_success = 0\n";
		output_file.close();
		return -1;
	}

	output_file << "download_parse_success = 1\n";
	pos = resultstr.find("http://", pos);
	size_t end = resultstr.find('\"', pos);
	std::string download_url = resultstr.substr(pos, end - pos);
	output_file << "download_url = " + download_url + '\n';

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
	output_file << "download_filename = " << filename << filetype << '\n';
	output_file.close();

}

