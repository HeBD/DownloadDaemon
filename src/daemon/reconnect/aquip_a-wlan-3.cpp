#include <string>
#include <curl/curl.h>

using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

extern "C" void reconnect(std::string host, std::string user, std::string password) {
	CURL* handle = curl_easy_init();
	std::string url = "http://" + host + "/goform/SysStatusHandle";
	std::string final = url + "?action=4";
	std::string auth = user + ':' + password;
	std::string resultstr;
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

