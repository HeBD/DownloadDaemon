#ifndef PACKAGE_CONTAINER_H_INCLUDED
#define PACKAGE_CONTAINER_H_INCLUDED

#include <config.h>
#include <string>
#include <vector>
#include "download.h"
#include "download_container.h"

#ifndef HAVE_UINT64_T
	#define uint64_t double
#endif

typedef std::pair<int, int> dlindex;

class package_container {
	public:

	/** Simple constructor - creates a list and adds the first, invisible global package where downloads with an insecure status are put */
	package_container();

	/** destroys and cleans up */
	~package_container();

	/** Read the package list from the dlist file
	*	@param filename Path to the dlist file
	*	@returns true on success
	*/
	int from_file(const char* filename);

	/** creates a package in the list
	*	@param pkg_name Name of the package
	*	@returns the package ID of the newly created package
	*/
	int add_package(std::string pkg_name);

	/** adds an already existing package to the list
	*	@param pkg_name Name of the package
	*	@param downloads the package itsself
	*	@returns the ID of the added package
	*/
	int add_package(std::string pkg_name, download_container* downloads);

	/** adds a download to an existing package
	*	@param dl Download object (has to be created with new. the class will take care of proper destruction)
	*	@param pkg_id the ID of the package where the download should be added
	*	@returns LIST_SUCCESS, LIST_ID
	*/
	int add_dl_to_pkg(download* dl, int pkg_id);

	/** deletes a package and all downloads inside it
	*	@param pkg_id ID of the package
	*/
	void del_package(int pkg_id);

	/** Checks if there are any packages in the download list
	*	@returns true if it's empty, else false
	*/
	bool empty();

	/** returns the index of the next downloadable file over all packages
	*	@returns the index
	*/
	dlindex get_next_downloadable();


	void set_url(dlindex dl, std::string url);
	std::string get_url(dlindex dl);

	void set_title(dlindex dl, std::string title);
	std::string get_title(dlindex dl);

	void set_add_date(dlindex dl, std::string add_date);
	std::string get_add_date(dlindex dl);

	void set_downloaded_bytes(dlindex dl, uint64_t bytes);
	uint64_t get_downloaded_bytes(dlindex dl);

	void set_size(dlindex dl, uint64_t size);
	uint64_t get_size(dlindex dl);

	void set_wait(dlindex dl, int seconds);
	int get_wait(dlindex dl);

	void set_error(dlindex dl, plugin_status error);
	plugin_status get_error(dlindex dl);

	void set_output_file(dlindex dl, std::string output_file);
	std::string get_output_file(dlindex dl);

	void set_running(dlindex dl, bool running);
	bool get_running(dlindex dl);

	void set_need_stop(dlindex dl, bool need_stop);
	bool get_need_stop(dlindex dl);

	void set_status(dlindex dl, download_status status);
	download_status get_status(dlindex dl);

	void set_speed(dlindex dl, int speed);
	int get_speed(dlindex dl);

	void set_can_resume(dlindex dl, bool can_resume);
	bool get_can_resume(dlindex dl);

	void set_proxy(dlindex dl, std::string proxy);
	std::string get_proxy(dlindex dl);

	CURL* get_handle(dlindex dl);

	/** inits a download-handle
	*	@param id ID of the download
	*/
	void init_handle(dlindex dl);

	/** cleans up the handle for a download
	*	@param id ID of the download
	*/
	void cleanup_handle(dlindex dl);

	/** strip the host from the URL
	*	@param dl Download from which to get the host
	*	@returns the hostname
	*/
	std::string get_host(dlindex dl);

	/** Prepares a download (calls the plugin, etc)
	*	@param dl Download id to prepare
	*	@param poutp plugin_output structure, will be filled in by the plugin
	*	@returns LIST_ID, LIST_SUCCESS
	*/
	int prepare_download(dlindex dl, plugin_output &poutp);

	/** Returns info about a plugin
	*	@param dl Download toget info from
	*	@returns the info
	*/
	plugin_output get_hostinfo(dlindex dl);

	/** post-processes a finished download by calling the plugin and do what it says
	*	@param id ID of the download
	*/
	void post_process_download(dlindex dl);

	/** returns the total number of downloads
	*	@returns the number
	*/
	int total_downloads();

	/** decrease all wait-times by 1 */
	void decrease_waits();

	/** removes downloads that have the DOWNLOAD_DELETED status and are not in use any more */
	void purge_deleted();

	/** creates the list that should be sent to the client
	*	@returns the list
	*/
	std::string create_client_list();

	/** Checks if a given URL is in the list already
	*	@param url URL to check for
	*	@returns True if it's already in there
	*/
	bool url_is_in_list(std::string url);

	enum direction {DIRECTION_UP = 0, DIRECTION_DOWN};
	/** moves a download up or down
	*	@param dl Download to move
	*	@param d Direction (package_container::DIRECTION_UP or package_container::DIRECTION_DOWN)
	*/
	void move_dl(dlindex dl, package_container::direction d);

	/** moves a package up or down
	*	@param id package to move
	*	@param d Direction (package_container::DIRECTION_UP or package_container::DIRECTION_DOWN)
	*/
	void move_pkg(int dl, package_container::direction d);

	/** dumps the current list to the dlist file
	*	@param do_lock if called from a locking package_container member functon, this needs to be false
	*/
	void dump_to_file(bool do_lock = true);

	/**	waits for the download dl as long as the wait-time is > 0. Breaks if it's set to 0 from another thread
	*	@param dl Index of the download
	*/
	void wait(dlindex dl);

	/** checks for the next free download ID that should be used for a download that is added
	*	@param set to false if you already locked ALL package's download_mutex mutexes
	*	@returns the ID
	*/
	int get_next_download_id(bool lock_download_mutex = true);

	/** Finds the package ID of the package that contains a download
	*	@param download_id the Download to search for
	*	@returns the package id
	*/
	int pkg_that_contains_download(int download_id);

	/** Checks if a given package exists
	*	@param id ID of the package
	*	@returns true if it exists
	*/
	bool pkg_exists(int id);

	/** Sets the next proxy from list to the download
	*	@param id ID of the download
	*	@returns 1 If the next proxy has been set
	*		 	 2 if all proxys have been tried already
	*			 3 if there are no proxys at all
	*/
	int set_next_proxy(dlindex id);

	/** Checks if all downloads in a package are finished
	*	@param id ID of the package
	*	@returns true if finished, false if not
	*/
	bool package_finished(int id);

	/** Extracts the archives from a package
	*	@param id ID of the package
	*/
	void extract_package(int id);


private:
	typedef std::vector<download_container*>::iterator iterator;

	iterator package_by_id(int pkg_id);
	int get_next_id();
	std::string get_plugin_file(download_container::iterator dlit);

	/** Checks if a reconnect is currently needed
	*	@returns true if yes
	*/
	bool reconnect_needed();

	/** Does the real work when reconnecting */
	void do_reconnect();

	/** give all downloads that have the ID -1 a proper ID. A plugin can not find out the download-id
	*	of a download it wants to add to its container. So it sets the ID to -1. When the plugin returns,
	*	the downloads then get a proper ID with this function.
	*/
	void correct_invalid_ids();

	std::vector<download_container*> packages;
	std::vector<download_container*> packages_to_delete;

	std::mutex mx;
	std::mutex plugin_mutex;
	std::string list_file;
	bool is_reconnecting;
};



#endif // PACKAGE_CONTAINER_H_INCLUDED
