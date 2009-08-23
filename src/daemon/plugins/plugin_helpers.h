#ifndef PLUGIN_HELPERS_H_INCLUDED
#define PLUGIN_HELPERS_H_INCLUDED

#include "../dl/download.h"

/*
	The following types are imported from download.h and may be/have to be used for writing plugins:
	enum plugin_status { PLUGIN_SUCCESS = 1, PLUGIN_ERROR, PLUGIN_LIMIT_REACHED, PLUGIN_FILE_NOT_FOUND, PLUGIN_CONNECTION_ERROR, PLUGIN_SERVER_OVERLOADED,
					 PLUGIN_MISSING, PLUGIN_INVALID_HOST, PLUGIN_INVALID_PATH, PLUGIN_CONNECTION_LOST, PLUGIN_WRITE_FILE_ERROR };

struct plugin_output {
	std::string download_url;
	std::string download_filename;
	bool allows_resumption;
	bool allows_multiple;
};

struct plugin_input {
	std::string premium_user;
	std::string premium_password;
};
*/

void set_wait_time(download &dl, int seconds) {
	dl.wait_seconds = seconds;
}

int get_wait_time(download &dl) {
	return dl.wait_seconds;
}

const char* get_url(download &dl) {
	return dl.url.c_str();
}

#endif // PLUGIN_HELPERS_H_INCLUDED
