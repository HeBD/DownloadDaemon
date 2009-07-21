#ifndef DOWNLOAD_H_
#define DOWNLOAD_H_

#include <string>
#include <fstream>

enum download_error { NO_ERROR = 1, MISSING_PLUGIN, INVALID_HOST, INVALID_PLUGIN_PATH, PLUGIN_ERROR, CONNECTION_LOST, FILE_NOT_FOUND };
enum auth_type { NO_AUTH = 1, HTTP_AUTH };
enum download_status { DOWNLOAD_PENDING = 1, DOWNLOAD_INACTIVE, DOWNLOAD_FINISHED, DOWNLOAD_RUNNING, DOWNLOAD_WAITING };

struct parsed_download {
	std::string download_url;
	std::string cookie_file;
	int wait_before_download;
	bool download_parse_success;
	std::string download_parse_errmsg;
	int download_parse_wait;
	int plugin_return_val;
};

struct hostinfo {
	bool offers_premium;
	bool allows_multiple_downloads_free;
	bool allows_multiple_downloads_premium;
	bool allows_download_resumption_free;
	bool allows_download_resumption_premium;
	bool requires_cookie;
	auth_type premium_auth_type;
};



class download {
public:
	/** Normal constructor
	* @param url Download URL
	* @param plugindir Folder to the plugins
	* @param next_id The ID this download should get
	*/
	download(std::string &url, int next_id);

	/** Constructor from a serialized download from file
	* @param serializedDL Serialized Download string
	*/
	download(std::string &serializedDL);

	/** Simple constructor
	*/
	download() {}

	/** Initialize a download object with a serialized download string
	* @param serializedDL Serialized download
	*/
	void from_serialized(std::string &serializedDL);

	/** execute the correct plugin with parameters and return information on the download
	* @param parsed_dl Structure in which all information required to start the download is stored
	* @return success status
	*/
	int get_download(parsed_download &parsed_dl);

	/** Serialize the download object to store it in the file
	* @return The serialized string
	*/
	std::string serialize();

	/** Find out the hoster (needed to call the correct plugin)
	* @return host-string
	*/
	std::string get_host();

	hostinfo get_hostinfo();

	const char* get_error_str();
	const char* get_status_str();

	std::string url;
	std::string comment;

	download_status status;
	std::string add_date;
	int id;
	long downloaded_bytes;
	long size;
	int wait_seconds;
	download_error error;


private:
	//std::string int_to_string(int i);

};

#endif /*DOWNLOAD_H_*/
