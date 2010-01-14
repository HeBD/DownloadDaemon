#include "recursive_parser.h"
#include "download.h"
#include "../global.h"
#include <string>
#include <vector>
#include <curl/curl.h>
using namespace std;

recursive_parser::recursive_parser(std::string url) : folder_url(url) {
}

void recursive_parser::add_to_list() {
	deep_parse(folder_url);
}

void recursive_parser::deep_parse(std::string url) {
	std::string list;
	CURL* handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, recursive_parser::to_string_callback);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &list);
	curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, 1024);
	curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, 30);
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	std::vector<std::string> urls = parse_list(list);
	for(std::vector<std::string>::iterator it = urls.begin(); it != urls.end(); ++it) {
		if((*it)[it->size() - 1] == '/') {
			deep_parse(url + *it);
		} else {
			download dl(url + *it, global_download_list.get_next_id());
			global_download_list.add_download(dl);
		}
	}
}

std::vector<std::string> recursive_parser::parse_list(std::string list) {
	std::vector<std::string> split_list;
	std::vector<std::string> files;
	size_t last_found = 0;
	for(size_t i = 0; i < list.length(); ++i) {
		if(list[i] == '\n') {
			split_list.push_back(list.substr(last_found, i));
			last_found = i + 1;
		}
	}

	for(vector<string>::iterator it = split_list.begin(); it != split_list.end(); ++it) {
		bool is_folder = false;
		if((*it)[0] == 'd') {
			is_folder = true;
		}
		// we need to get to column 8 of that folder
		int col_start = 0, curr_col = 0;
		bool ignore_next_space = false;
		for(size_t i = 0; i < it->size(); ++i) {
			if(isspace((*it)[i]) && ignore_next_space == false) {
				ignore_next_space = true;
			} else if(!isspace((*it)[i]) && ignore_next_space == true) {
				ignore_next_space = false;
				++curr_col;
				if(curr_col == 8) {
					col_start = i;
					break;
				}
			}
		}
		string to_add = it->substr(col_start);
		to_add = to_add.substr(0, to_add.find('\n'));
		if(is_folder) {
			to_add += '/';
		}
		files.push_back(to_add);
	}
	return files;
}

size_t recursive_parser::to_string_callback(void *buffer, size_t size, size_t nmemb, void *userp) {
	string* strptr = (string*)userp;
	strptr->append((char*)buffer, nmemb);
	return nmemb;
}

