#include<iostream>
#include<curl/curl.h>
#include<cstdlib>
using namespace std;

struct curl_slist *curl_slist_append_s(struct curl_slist *list, const std::string &string ) {
	return curl_slist_append(list, string.c_str());
}

CURLcode curl_easy_setopt_s(CURL *handle, CURLoption option, string str) {
	return curl_easy_setopt(handle, option, str.c_str());
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

extern "C" void reconnect(std::string host, std::string user, std::string password) {
	CURL* handle = curl_easy_init();
	std::string resultstr;
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &resultstr);

	{
		curl_easy_reset(handle);
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, &resultstr);
		struct curl_slist *header = NULL;
		curl_easy_setopt_s(handle, CURLOPT_URL, "http://" + host + "/cgi-bin/login.exe");
		curl_easy_setopt(handle, CURLOPT_POST, 1);
		curl_easy_setopt(handle, CURLOPT_POSTFIELDS, "totalMSec=1253574018.503&pws=d41d8cd98f00b204e9800998ecf8427e&pws_temp=");
		curl_easy_setopt(handle, CURLOPT_HTTPHEADER, header);
		curl_easy_perform(handle);
		curl_slist_free_all(header);
	}
	{
		curl_easy_reset(handle);
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, &resultstr);
		struct curl_slist *header = NULL;
		curl_easy_setopt_s(handle, CURLOPT_URL, "http://" + host + "/cgi-bin/restart.exe");
		curl_easy_setopt(handle, CURLOPT_POST, 1);
		curl_easy_setopt(handle, CURLOPT_POSTFIELDS, "page=tools_gateway&logout=");
		curl_easy_setopt(handle, CURLOPT_HTTPHEADER, header);
		curl_easy_perform(handle);
		curl_slist_free_all(header);
	}
	sleep(40);
}
