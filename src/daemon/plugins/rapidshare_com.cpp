#include <curl/curl.h>
#include <string>
#include <cstdlib>
#include <fstream>

using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	string *blubb = (string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
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
	int success = curl_easy_perform(handle);

	if(success != 0) {
		output_file << "download_parse_success = 0\n";
		output_file << "download_parse_errmsg = CONNECTION_FAILED\n";
		output_file.close();
		return 0;
	}

	if(resultstr.find("The file could not be found") != string::npos) {
		output_file << "download_parse_success = 0\n";
		output_file << "download_parse_errmsg = FILE_NOT_FOUND\n";
		output_file.close();
		return 0;
	}

	unsigned int pos;
	if((pos = resultstr.find("<form id=\"ff\"")) == string::npos) {
		output_file.close();
		return -1;
	}
	try {
		string url;
		pos = resultstr.find("http://", pos);
		unsigned int end = resultstr.find('\"', pos);

		url = resultstr.substr(pos, end - pos);

		resultstr.clear();
		curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
		curl_easy_setopt(handle, CURLOPT_POST, 1);
		curl_easy_setopt(handle, CURLOPT_POSTFIELDS, "dl.start=\"Free\"");
		curl_easy_perform(handle);

		if(resultstr.find("Or try again in about") != string::npos) {
			pos = resultstr.find("Or try again in about");
			pos = resultstr.find("about", pos);
			pos = resultstr.find(' ', pos);
			++pos;
			end = resultstr.find(' ', pos);
			output_file << "download_parse_success = 0\n";
			output_file << "download_parse_errmsg = LIMIT_REACHED\n";
			string have_to_wait(resultstr.substr(pos, end - pos));
			unsigned int wait_secs = atoi(have_to_wait.c_str()) * 60;
			output_file << "download_parse_wait = " << wait_secs;
			output_file.close();
			return 0;
		} else if(resultstr.find("Please try again in 2 minutes") != string::npos) {
			output_file << "download_parse_success = 0\n";
			output_file << "download_parse_errmsg = SERVER_OVERLOAD\n";
			output_file << "download_parse_wait = 120\n";
			output_file.close();
			return 0;
		} else {
			output_file << "download_parse_success = 1\n";

			if((pos = resultstr.find("var c")) == string::npos) {
				// Unable to get the wait time, will wait 135 seconds
				output_file << "wait_before_download = 135" << '\n';
			} else {
				pos = resultstr.find("=", pos) + 1;
				end = resultstr.find(";", pos) - 1;
				output_file << "wait_before_download = " << atoi(resultstr.substr(pos, end).c_str()) << '\n';
			}

			if((pos = resultstr.find("<form name=\"dlf\"")) == string::npos) {
				return -1;
			}

			pos = resultstr.find("http://", pos);
			end = resultstr.find('\"', pos);

			output_file << "download_url = " << resultstr.substr(pos, end - pos) << '\n';
			output_file.close();
			return 0;
		}
	} catch (...) {
		output_file.close();
		return -1;
		}
}
