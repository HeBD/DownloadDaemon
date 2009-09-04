#ifndef PLUGIN_HELPERS_H_INCLUDED
#define PLUGIN_HELPERS_H_INCLUDED

#include "../dl/download.h"
#include "../dl/download_container.h"
#include <boost/bind.hpp>


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

download_container *list;
int dlid;
plugin_status plugin_exec(plugin_input &pinp, plugin_output &poutp);

extern "C" plugin_status plugin_exec_wrapper(download_container& dlc, int id, plugin_input& pinp, plugin_output& poutp) {
	list = &dlc;
	dlid = id;
	return plugin_exec(pinp, poutp);
}

void set_wait_time(int seconds) {
	list->set_int_property(dlid, DL_WAIT_SECONDS, seconds);
}

int get_wait_time() {
	return list->get_int_property(dlid, DL_WAIT_SECONDS);
}

const char* get_url() {
	return list->get_string_property(dlid, DL_URL).c_str();
}

#endif // PLUGIN_HELPERS_H_INCLUDED
