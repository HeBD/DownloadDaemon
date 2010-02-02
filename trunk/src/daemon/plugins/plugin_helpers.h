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
Also, the functions below may help you writing your plugin by implementing some basic things you might need and simplifying tasks
you would usually do with the result of get_dl_container().
*/




/** Set the wait-time for the currently active download. Set this whenever you have to wait for the host. DownloadDaemon will count down
 *	that time until it reaches 0. This is just for the purpose of displaying a status to the user.
 *	@param seconds Seconds to wait
 */
void set_wait_time(int seconds);

/** returns the currently set wait-time for your download
 *	@returns wait time in seconds
 */
int get_wait_time();

/** Get the URL for your download, which was entered by the user
 *	@returns the url
 */
const char* get_url();

/** allows you to change the URL in the download list
 *	@param url URL to set
 */
void set_url(const char* url);

/** gives you a pointer to the main download-list. With this, you can do almost anything with any download
 *	@returns pointer to the main download list
 */
download_container* get_dl_container();

/** this returns the curl-handle which will be used for downloading later. You may need it for setting special options like cookies, etc.
 *	@returns the curl handle
 */
CURL* get_handle();

/** passing a download_container object to that function will delete the current download and replace it by all the links specified in lst.
 *	this is mainly for decrypter-plugins for hosters that contain several download-links of other hosters
 *	@param lst the download list to use for replacing
 */
void replace_this_download(download_container &lst);

/** remove white-spaces from the beginning and ending of str
 *	@param str String from which whitespaces will be stripped
 */
void trim_string(std::string &str);

/** replace all occurences of old with new_s in str
 *	@param str string to use for replacing
 *	@param old string that should be replaced
 *	@param new_s string which will be inserted instead of old
 */
void replace_all(std::string& str, const std::string& old, const std::string& new_s);

/** this function replaces some html-encoded characters with native ansi characters (eg replaces &quot; with " and &lt; with <)
 *	@param s string in which to replace
 */
void replace_html_special_chars(std::string& s);

/** convert anything with an overloaded operator << to string (eg int, long, double, ...)
 *	@param p1 any type to convert into std::string
 *	@returns the result of the conversion
 */
template <class PARAM>
std::string convert_to_string(PARAM p1);


/////////////////////// IMPLEMENTATION ////////////////////////


download_container *list;
int dlid;
plugin_status plugin_exec(plugin_input &pinp, plugin_output &poutp);

/** This function is just a wrapper for defining globals and calling your plugin */
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

void trim_string(std::string &str) {
	while(str.length() > 0 && isspace(str[0])) {
		str.erase(str.begin());
	}
	while(str.length() > 0 && isspace(*(str.end() - 1))) {
		str.erase(str.end() -1);
	}
}

void replace_all(std::string& str, const std::string& old, const std::string& new_s) {
	size_t n;
	while((n = str.find(old)) != std::string::npos) {
		str.replace(n, old.length(), new_s);
	}
}

void replace_html_special_chars(std::string& s) {
	replace_all(s, "&quot;", "\"");
	replace_all(s, "&lt;", "<");
	replace_all(s, "&gt;", ">");
	replace_all(s, "&apos;", "'");
	replace_all(s, "&amp;", "&");
}


template <class PARAM>
std::string convert_to_string(PARAM p1) {
	std::stringstream ss;
	ss << p1;
	return ss.str();
}



// I know, this looks ugly, but it does the job and seems to be okay in this case
#include "../dl/download_container.cpp"
#include "../dl/download.cpp"

#endif // PLUGIN_HELPERS_H_INCLUDED
