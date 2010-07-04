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

#include <sstream>
#include <config.h>
#include "../dl/download.h"
#include "../dl/download_container.h"
#include "captcha.h"

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
	filesize_t file_size;
	plugin_status file_online;
};

struct plugin_input {
	std::string premium_user;
	std::string premium_password;
	std::string url;
};

You also might be interested in checking ../dl/download_container.h to see what you can do with the result of get_dl_container().
Also, the functions below may help you writing your plugin by implementing some basic things you might need and simplifying tasks
you would usually do with the result of get_dl_container().
*/

// forward declaration
plugin_status plugin_exec(plugin_input &pinp, plugin_output &poutp);
namespace PLGFILE {

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

	#include <vector>
	/** Splits a string into many strings, seperating them with the seperator
	 *  @param inp_string string to split
	 *	@param seperator Seperator to use for splitting
	 *	@returns Vector of all the strings
	 */
	std::vector<std::string> split_string(const std::string& inp_string, const std::string& seperator);


	/////////////////////// IMPLEMENTATION ////////////////////////


	download_container *dl_list;
	download           *dl_ptr;
	int dlid;
	int max_retrys;
	int retry_count;
	std::string gocr;
	std::string host;
	std::string share_directory;
	std::mutex p_mutex;

	void set_wait_time(int seconds) {
		dl_ptr->set_wait(seconds);
	}

	int get_wait_time() {
		return dl_ptr->get_wait();
	}

	const char* get_url() {
		return dl_ptr->get_url().c_str();
	}

	void set_url(const char* url) {
		dl_ptr->set_url(url);
	}

	download_container* get_dl_container() {
		return dl_list;
	}

	CURL* get_handle() {
		return dl_ptr->get_handle();
	}

	void replace_this_download(download_container &lst) {
		dl_list->insert_downloads(dl_list->get_list_position(dlid), lst);
		dl_list->set_status(dlid, DOWNLOAD_DELETED);
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

	std::vector<std::string> split_string(const std::string& inp_string, const std::string& seperator) {
		std::vector<std::string> ret;
		size_t n = 0, last_n = 0;
		while(true) {
			n = inp_string.find(seperator, n);
			ret.push_back(inp_string.substr(last_n, n - last_n));
			if(n == std::string::npos) break;
			n += seperator.size();
			last_n = n;

		}
		return ret;
	}

}

using namespace PLGFILE;

#ifdef IS_PLUGIN
int download_container::add_download(const std::string& url, const std::string& title) {
	download* dl = new download(url);
	dl->set_title(title);
	dl->set_parent(container_id);
	int ret = add_download(dl, -1);
	if(ret != LIST_SUCCESS) delete dl;
	return ret;
}
#endif

/** This function is just a wrapper for defining globals and calling your plugin */
extern "C" plugin_status plugin_exec_wrapper(download_container* dlc, download* pdl, int id, plugin_input& pinp, plugin_output& poutp,
                                             int max_captcha_retrys, const std::string &gocr_path, const std::string &root_dir) {
	std::lock_guard<std::mutex> lock(p_mutex);
	dl_ptr = pdl;
	dl_list = dlc;
	dlid = id;
	max_retrys = max_captcha_retrys;
	gocr = gocr_path;
	host = dlc->get_host(id);
	retry_count = 0;
	share_directory = root_dir;
	return plugin_exec(pinp, poutp);
}

#ifdef PLUGIN_CAN_PRECHECK
bool get_file_status(plugin_input &inp, plugin_output &outp);
extern "C" bool get_file_status_init(download_container &dlc, download* pdl, int id, plugin_input &inp, plugin_output &outp) {
	std::lock_guard<std::mutex> lock(p_mutex);
	dl_list = &dlc;
        dl_ptr = pdl;
	dlid = id;
	return get_file_status(inp, outp);
}
#endif

#ifdef PLUGIN_WANTS_POST_PROCESSING
void post_process_download(plugin_input&);
extern "C" void post_process_dl_init(download_container& dlc, download *pdl, int id, plugin_input& pinp) {
	std::lock_guard<std::mutex> lock(p_mutex);
	dl_list = &dlc;
        dl_ptr = pdl;
	dlid = id;
	host = dlc.get_host(id);
	post_process_download(pinp);
}
#endif

#if defined (__CYGWIN__) || defined (__APPLE__)
// I know, this looks ugly, but it does the job and seems to be okay in this case. It's only on cygwin
// because windows doesn't support dlls with missing symbols. They all need to be available at compile-time.
// This does the job until I find the time to declare DD's functions and classes __declspec(dllexport)
	#include "../dl/download_container.cpp"
	#include "../dl/download.cpp"
#endif
#include "captcha.cpp"

#endif // PLUGIN_HELPERS_H_INCLUDED
