/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef PLUGIN_HELPERS_H_INCLUDED
#define PLUGIN_HELPERS_H_INCLUDED
#define IS_PLUGIN

#include "../dl/download.h"
#include "../dl/download_container.h"
#include <boost/bind.hpp>


/*
The following types are imported from download.h and may be/have to be used for writing plugins:
enum plugin_status { PLUGIN_SUCCESS = 1, PLUGIN_ERROR, PLUGIN_LIMIT_REACHED, PLUGIN_FILE_NOT_FOUND, PLUGIN_CONNECTION_ERROR, PLUGIN_SERVER_OVERLOADED,
					 PLUGIN_INVALID_HOST, PLUGIN_INVALID_PATH, PLUGIN_CONNECTION_LOST, PLUGIN_WRITE_FILE_ERROR, PLUGIN_AUTH_FAIL };

struct plugin_output {
	std::string download_url;
	std::string download_filename;
	bool allows_resumption;
	bool allows_multiple;
	bool offers_premium;
};

struct plugin_input {
	std::string premium_user;
	std::string premium_password;
};

You also might be interested in checking ../dl/download_container.h to see what you can do with the result of get_dl_container().
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

void set_url(const char* url) {
	list->set_string_property(dlid, DL_URL, url);
}

download_container* get_dl_container() {
	return list;
}

CURL* get_handle() {
	return list->get_pointer_property(dlid, DL_HANDLE);
}

void replace_this_download(download_container &lst) {
	list->insert_downloads(list->get_list_position(dlid), lst);
	list->set_int_property(dlid, DL_STATUS, DOWNLOAD_DELETED);
}



// I know, this looks ugly, but it does the job and seems to be okay in this case
#include "../dl/download_container.cpp"
#include "../dl/download.cpp"

#endif // PLUGIN_HELPERS_H_INCLUDED
