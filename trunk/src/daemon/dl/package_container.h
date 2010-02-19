#ifndef PACKAGE_CONTAINER_H_INCLUDED
#define PACKAGE_CONTAINER_H_INCLUDED

#include <string>
#include <vector>
#include "download.h"
#include "download_container.h"

typedef std::pair<int, int> dlindex;

class package_container {
	public:


	package_container();

	~package_container();

	/** Read the package list from the dlist file
	*	@param filename Path to the dlist file
	*	@returns true on success
	*/
	int from_file(const char* filename);

	int add_package(std::string pkg_name);
	int add_package(std::string pkg_name, download_container* downloads);
	int add_dl_to_pkg(download* dl, int pkg_id);
	void del_package(int pkg_id);

	bool empty();

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

	CURL* get_handle(dlindex dl);
	void init_handle(dlindex dl);
	void cleanup_handle(dlindex dl);

	std::string get_host(dlindex dl);

	int prepare_download(dlindex dl, plugin_output &poutp);
	plugin_output get_hostinfo(dlindex dl);
	void post_process_download(dlindex dl);

	int total_downloads();

	void decrease_waits();
	void purge_deleted();

	std::string create_client_list();

	bool url_is_in_list(std::string url);

	enum direction {DIRECTION_UP = 0, DIRECTION_DOWN};
	void move_dl(dlindex dl, package_container::direction d);
	void move_pkg(int dl, package_container::direction d);


	void dump_to_file(bool do_lock = true);

	void wait(dlindex dl);

	int get_next_download_id(bool lock_download_mutex = true);

	int pkg_that_contains_download(int download_id);

	bool pkg_exists(int id);

private:
	typedef std::vector<download_container*>::iterator iterator;

	iterator package_by_id(int pkg_id);
	int get_next_id();
	std::string get_plugin_file(download_container::iterator dlit);
	bool reconnect_needed();
	void do_reconnect();
	void correct_invalid_ids();

	std::vector<download_container*> packages;
	std::mutex mx;
	std::mutex plugin_mutex;
	std::string list_file;
	bool is_reconnecting;
};



#endif // PACKAGE_CONTAINER_H_INCLUDED
