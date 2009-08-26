#ifndef DOWNLOAD_H_
#define DOWNLOAD_H_

#include <string>
#include <fstream>
#include <curl/curl.h>
#include <boost/thread.hpp>
#include "../../lib/mt_string/mt_string.h"

enum download_status { DOWNLOAD_PENDING = 1, DOWNLOAD_INACTIVE, DOWNLOAD_FINISHED, DOWNLOAD_RUNNING, DOWNLOAD_WAITING, DOWNLOAD_DELETED, DOWNLOAD_RECONNECTING };
enum plugin_status { PLUGIN_SUCCESS = 1, PLUGIN_ERROR, PLUGIN_LIMIT_REACHED, PLUGIN_FILE_NOT_FOUND, PLUGIN_CONNECTION_ERROR, PLUGIN_SERVER_OVERLOADED,
					 PLUGIN_MISSING, PLUGIN_INVALID_HOST, PLUGIN_INVALID_PATH, PLUGIN_CONNECTION_LOST, PLUGIN_WRITE_FILE_ERROR };

struct plugin_output {
	mt_string download_url;
	mt_string download_filename;
	bool allows_resumption;
	bool allows_multiple;
};

struct plugin_input {
	mt_string premium_user;
	mt_string premium_password;
};

class download {
public:
	/** Normal constructor
	* @param url Download URL
	* @param next_id The ID this download should get
	*/
	download(mt_string &url, int next_id);

	/** Constructor from a serialized download from file
	* @param serializedDL Serialized Download string
	*/
	download(mt_string &serializedDL);

	/** Copy constructor because we are not allowed to copy a boost::mutex
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
	void from_serialized(mt_string &serializedDL);

	/** execute the correct plugin with parameters and return information on the download
	* @returns success status
	*/
	plugin_status get_download(plugin_output &outp);

	/** Serialize the download object to store it in the file
	* @returns The serialized string
	*/
	mt_string serialize();

	/** Find out the hoster (needed to call the correct plugin)
	* @returns host-string
	*/
	mt_string get_host();

	/** Get the defines from above as a string literal
	* @returns the resulting error string
	*/
	const char* get_error_str();

	/** Get the defines from above as a string literal
	* @returns the resulting status string
	*/
	const char* get_status_str();

	/** Sets the status of a download
	* @param st desired status
	* @param force If this is set to true, no checking will happen before setting the status. this should usually NOT be done, because in some cases setting a status
	*			   needs special actions to take place before. Only use if you are 100% sure that you have to
	*/
	void set_status(download_status st, bool force = false);

	/** Returns the status of a download
	* @returns Download status
	*/
	download_status get_status();

	plugin_output get_hostinfo();

	const mt_string& get_url() { boost::mutex::scoped_lock lock(download_mutex); return url; }
	void set_url(const mt_string &str) { boost::mutex::scoped_lock lock(download_mutex); url = str; }

	const mt_string& get_comment() { boost::mutex::scoped_lock lock(download_mutex); return comment; }
	void set_comment(const mt_string &str) { boost::mutex::scoped_lock lock(download_mutex); comment = str; }

	const mt_string& get_add_date() { boost::mutex::scoped_lock lock(download_mutex); return add_date; }
	void set_add_date(const mt_string &str) { boost::mutex::scoped_lock lock(download_mutex); add_date = str; }

	int get_id() { boost::mutex::scoped_lock lock(download_mutex); return id; }
	void set_id(int id) { boost::mutex::scoped_lock lock(download_mutex); this->id = id; }

	long get_downloaded_bytes() { boost::mutex::scoped_lock lock(download_mutex); return downloaded_bytes; }
	void set_downloaded_bytes(long downloaded_bytes) { boost::mutex::scoped_lock lock(download_mutex); this->downloaded_bytes = downloaded_bytes; }

	long get_size() { boost::mutex::scoped_lock lock(download_mutex); return size; }
	void set_size(long size) { boost::mutex::scoped_lock lock(download_mutex); this->size = size; }

	int get_wait_seconds() { boost::mutex::scoped_lock lock(download_mutex); return wait_seconds; }
	void set_wait_seconds(int wait_seconds) { boost::mutex::scoped_lock lock(download_mutex); this->wait_seconds = wait_seconds; }

	plugin_status get_plugin_status() { boost::mutex::scoped_lock lock(download_mutex); return error; }
	void set_plugin_status(plugin_status st) { boost::mutex::scoped_lock lock(download_mutex); this->error = st; }

	CURL* get_handle() { boost::mutex::scoped_lock lock(download_mutex); return handle; }

	const mt_string& get_output_file() { boost::mutex::scoped_lock lock(download_mutex); return output_file; }
	void set_output_file(const mt_string &str) { boost::mutex::scoped_lock lock(download_mutex); output_file = str; }

	friend bool operator<(const download& x, const download& y);

private:
	mt_string url;
	mt_string comment;


	mt_string add_date;
	int id;
	long downloaded_bytes;
	long size;
	int wait_seconds;
	plugin_status error;
	CURL* handle;
	mt_string output_file;

	boost::mutex download_mutex;
	download_status status;

};

bool operator<(const download& x, const download& y);

#endif /*DOWNLOAD_H_*/
