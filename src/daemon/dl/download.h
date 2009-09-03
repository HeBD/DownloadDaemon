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
	//plugin_status get_download(plugin_output &outp);

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
	void set_status(download_status st);

	/** Returns the status of a download
	* @returns Download status
	*/
	download_status get_status();

	plugin_output get_hostinfo();

	friend bool operator<(const download& x, const download& y);

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

	bool is_running;
	bool need_stop;
private:
	download_status status;


};

bool operator<(const download& x, const download& y);

#endif /*DOWNLOAD_H_*/
