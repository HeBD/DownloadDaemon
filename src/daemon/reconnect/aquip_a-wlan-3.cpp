#include "../../lib/mt_string/mt_string.h"
#include <curl/curl.h>

using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	mt_string *blubb = (mt_string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

extern "C" void reconnect(mt_string host, mt_string user, mt_string password) {
	CURL* handle = curl_easy_init();
	mt_string url = "http://" + host + "/goform/SysStatusHandle";
	mt_string final = url + "?action=4";
	mt_string auth = user + ':' + password;
	mt_string resultstr;
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &resultstr);
	curl_easy_setopt(handle, CURLOPT_URL, final.c_str());
	curl_easy_setopt(handle, CURLOPT_USERPWD, auth.c_str());
	curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
	curl_easy_perform(handle);

	final = url + "?action=3";
	curl_easy_setopt(handle, CURLOPT_URL, final.c_str());
	curl_easy_perform(handle);
}

