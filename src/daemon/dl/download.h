/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DOWNLOAD_H_
#define DOWNLOAD_H_

#include <config.h>
#include <string>
#include <fstream>

#include <curl/curl.h>

#ifndef USE_STD_THREAD
#include <boost/thread.hpp>
namespace std {
	using namespace boost;
}
#else
#include <thread>
#endif


enum download_status { DOWNLOAD_PENDING = 1, DOWNLOAD_INACTIVE, DOWNLOAD_FINISHED, DOWNLOAD_RUNNING, DOWNLOAD_WAITING, DOWNLOAD_DELETED, DOWNLOAD_RECONNECTING };
enum plugin_status { PLUGIN_SUCCESS = 1, PLUGIN_ERROR, PLUGIN_LIMIT_REACHED, PLUGIN_FILE_NOT_FOUND, PLUGIN_CONNECTION_ERROR, PLUGIN_SERVER_OVERLOADED,
					 PLUGIN_INVALID_HOST, PLUGIN_CONNECTION_LOST, PLUGIN_WRITE_FILE_ERROR, PLUGIN_AUTH_FAIL };

struct plugin_output {
	plugin_output() : allows_resumption(true), allows_multiple(true), offers_premium(false), file_size(0), file_online(PLUGIN_SUCCESS) {}
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

class download {
public:
	/** Normal constructor
	* @param url Download URL
	*/
	download(const std::string &dl_url);

	/** Constructor from a serialized download from file
	* @param serializedDL Serialized Download string
	*/
	//download(std::string &serializedDL);

	/** Copy constructor because of CURL* handles
	* @param dl Download object to copy
	*/
	download(const download& dl);

	/** needed because of boost::mutex noncopyability
	* @param dl download object to assign
	*/
	void operator=(const download& dl);

	/** Destructor to clean up the curl handle
	*/
	~download();

	/** Simple constructor
	*/
	download() {}

	/** Initialize a download object with a serialized download string
	* @param serializedDL Serialized download
	*/
	void from_serialized(std::string &serializedDL);

	/** Serialize the download object to store it in the file
	* @returns The serialized string
	*/
	std::string serialize();

	/** Find out the hoster (needed to call the correct plugin)
	* @returns host-string
	*/
	std::string get_host(bool do_lock = true);

	/** Get the defines from above as a string literal
	* @returns the resulting error string
	*/
	const char* get_error_str();

	/** Get the defines from above as a string literal
	* @returns the resulting status string
	*/
	const char* get_status_str();

	/** gives info about the host of this download (continue downloads, parallel downloads, premium, ...)
	*	@returns the info
	*/
	plugin_output get_hostinfo();

	void download_me();
	void download_me_worker();

	friend bool operator<(const download& x, const download& y);

	std::string get_url() const					{ std::lock_guard<std::recursive_mutex> lock(mx); return url; }
	void set_url(const std::string &new_url) 	{ std::lock_guard<std::recursive_mutex> lock(mx); url = new_url; }

	std::string get_title() const				{ std::lock_guard<std::recursive_mutex> lock(mx); return comment; }
	void set_title(const std::string &title) 	{ std::lock_guard<std::recursive_mutex> lock(mx); comment = title; }

	std::string get_add_date() const			{ std::lock_guard<std::recursive_mutex> lock(mx); return add_date; }
	void set_add_date(const std::string &new_add_date) { std::lock_guard<std::recursive_mutex> lock(mx); add_date = new_add_date; }

	int get_id() const							{ std::lock_guard<std::recursive_mutex> lock(mx); return id; }
	void set_id(int new_id) 					{ std::lock_guard<std::recursive_mutex> lock(mx); id = new_id; }

	filesize_t get_downloaded_bytes() const		{ std::lock_guard<std::recursive_mutex> lock(mx); return downloaded_bytes; }
	void set_downloaded_bytes(filesize_t b) 		{ std::lock_guard<std::recursive_mutex> lock(mx); downloaded_bytes = b; }

	filesize_t get_size() const					{ std::lock_guard<std::recursive_mutex> lock(mx); return size; }
	void set_size(filesize_t b) 					{ std::lock_guard<std::recursive_mutex> lock(mx); size = b; }

	int get_wait() const						{ std::lock_guard<std::recursive_mutex> lock(mx); return wait_seconds; }
	void set_wait(int w) 						{ std::lock_guard<std::recursive_mutex> lock(mx); wait_seconds = w; }

	plugin_status get_error() const				{ std::lock_guard<std::recursive_mutex> lock(mx); return error; }
	void set_error(plugin_status e)				{ std::lock_guard<std::recursive_mutex> lock(mx); error = e; }

	std::string get_filename() const			{ std::lock_guard<std::recursive_mutex> lock(mx); return output_file; }
	void set_filename(const std::string &fn) 	{ std::lock_guard<std::recursive_mutex> lock(mx); output_file = fn; }

	bool get_running() const					{ std::lock_guard<std::recursive_mutex> lock(mx); return is_running; }
	void set_running(bool r) 					{ std::lock_guard<std::recursive_mutex> lock(mx); is_running = r; }

	bool get_need_stop() const					{ std::lock_guard<std::recursive_mutex> lock(mx); return need_stop; }
	void set_need_stop(bool r) 					{ std::lock_guard<std::recursive_mutex> lock(mx); need_stop = r; }

	download_status get_status() const			{ std::lock_guard<std::recursive_mutex> lock(mx); return status; }
	void set_status(download_status st)			{ std::lock_guard<std::recursive_mutex> lock(mx); if(status == DOWNLOAD_DELETED) return;
												  status = st;
												  if(st == DOWNLOAD_INACTIVE || st == DOWNLOAD_DELETED) { need_stop = true; wait_seconds = 0; } }

	int get_speed()	const						{ std::lock_guard<std::recursive_mutex> lock(mx); return speed; }
	void set_speed(int s)						{ std::lock_guard<std::recursive_mutex> lock(mx); speed = s; }

	bool get_resumable() const					{ std::lock_guard<std::recursive_mutex> lock(mx); return can_resume; }
	void set_resumable(bool r) 					{ std::lock_guard<std::recursive_mutex> lock(mx); can_resume = r; }

	std::string get_proxy() const				{ std::lock_guard<std::recursive_mutex> lock(mx); return proxy; }
	void set_proxy(const std::string &p)		{ std::lock_guard<std::recursive_mutex> lock(mx); proxy = p; }

	CURL* get_handle() const					{ std::lock_guard<std::recursive_mutex> lock(mx); return handle; }
	void set_handle(CURL* h)					{ std::lock_guard<std::recursive_mutex> lock(mx); handle = h; }

	int get_parent() const						{ std::lock_guard<std::recursive_mutex> lock(mx); return parent; }
	void set_parent(int p)						{ std::lock_guard<std::recursive_mutex> lock(mx); parent = p; }

	bool get_prechecked() const					{ std::lock_guard<std::recursive_mutex> lock(mx); return already_prechecked; }
	void set_prechecked(bool p)					{ std::lock_guard<std::recursive_mutex> lock(mx); already_prechecked = p; }

	/** This function calls the plugin and checks if 1: the download is online and 2: what size it has.
	*	Then it sets the PLUGIN_* error and the filesize for the download
	*/
	void preset_file_status();

private:
	/** Sets the next proxy from list to the download
	*	@param id ID of the download
	*	@returns 1 If the next proxy has been set
	*		 	 2 if all proxys have been tried already
	*			 3 if there are no proxys at all
	*/
	int set_next_proxy();

	plugin_status prepare_download(plugin_output &poutp);
	#ifndef IS_PLUGIN
	/** post-processes a finished download by calling the plugin and do what it says */
	void post_process_download();
	#endif
	std::string get_plugin_file();
	void wait();

	std::string url;
	std::string comment;


	std::string add_date;
	int id;
	filesize_t downloaded_bytes;
	filesize_t size;
	int wait_seconds;
	plugin_status error;

	std::string output_file;

	bool is_running;
	bool need_stop;
	download_status status;
	int speed;
	bool can_resume;
	CURL* handle;
	std::string proxy;

	bool already_prechecked;

	int parent;
	mutable std::recursive_mutex mx;
};

#ifndef IS_PLUGIN
typedef std::pair<int, int> dlindex;

struct dl_cb_info {
	dl_cb_info() : resume_from(0), total_size(0), filename_from_effective_url(false), id(0, 0), curl_handle(NULL), break_reason(PLUGIN_SUCCESS) {}
	std::string     filename;
	std::string     download_dir;
	filesize_t      resume_from;
	filesize_t      total_size;
	std::fstream*   out_stream;
	bool            filename_from_effective_url;
	std::string     cache;
	dlindex         id;
	CURL*           curl_handle;
	plugin_status   break_reason;
};

#endif

bool operator<(const download& x, const download& y);

#endif /*DOWNLOAD_H_*/
