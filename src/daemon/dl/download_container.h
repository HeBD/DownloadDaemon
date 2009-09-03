#ifndef DOWNLOAD_CONTAINER_H_INCLUDED
#define DOWNLOAD_CONTAINER_H_INCLUDED

#include "download.h"
#include <string>
#include <vector>
#include <boost/thread.hpp>

enum property { DL_ID = 0, DL_DOWNLOADED_BYTES, DL_SIZE, DL_WAIT_SECONDS, DL_PLUGIN_STATUS, DL_STATUS, DL_IS_RUNNING, DL_NEED_STOP };
enum string_property { DL_URL = 20, DL_COMMENT, DL_ADD_DATE, DL_OUTPUT_FILE };
enum pointer_property { DL_HANDLE = 40 };
enum { LIST_SUCCESS = -20, LIST_PERMISSION, LIST_ID, LIST_PROPERTY };

class download_container {
public:
	/** Constructor taking a filename from which the list should be read
	*	@param filename Path to the dlist file
	*/
	download_container(const char* filename);

	/** simple constructor
	*/
	download_container() {}

	/** Read the download list from the dlist file
	*	@param filename Path to the dlist file
	*	@returns true on success
	*/
	int from_file(const char* filename);

	/** Check if download list is empty
	*	@returns True if empty
	*/
	bool empty();

	/** Returns the total amount of downloads in the list
	*	@returns download count
	*/
	int total_downloads();

	/** Moves a download upwards
	*	@param id download ID to move up
	*	@returns -1 if it's already the top download, -2 if the function fails because the dlist can't be written, 0 on success
	*/
	int move_up(int id);

	int move_down(int id);

	int activate(int id);
	int deactivate(int id);



	/** Gets the next downloadable item in the global download list (filters stuff like inactives, wrong time, etc)
 	*	@returns ID of the next download that can be downloaded or LIST_ID if there is none
 	*/
	int get_next_downloadable();

	int add_download(const download &dl);



	int set_string_property(int id, string_property prop, mt_string value);
	int set_int_property(int id, property prop, int value);
	int set_pointer_property(int id, pointer_property prop, void* value);

	mt_string get_string_property(int id, string_property prop);
	void* get_pointer_property(int id, pointer_property prop);
	int get_int_property(int id, property prop);

	int prepare_download(int dl, plugin_output &poutp);

	plugin_output get_hostinfo(int dl);

	mt_string get_host(int dl);

	void decrease_waits();

	void purge_deleted();

	mt_string create_client_list();

	/** Gets the lowest unused ID that should be used for the next download
	*	@returns ID
	*/
	int get_next_id();

	int stop_download(int id);

	mt_string list_file;

private:
	/** get an iterator to a download by giving an ID
	*	@param id download ID to search for
	*	@returns Iterator to this id
	*/
	std::vector<download>::iterator get_download_by_id(int id);

	/** Dumps the download list from RAM to the file
	*	@returns true on success
	*/
	bool dump_to_file();

	/** Returns the amount of running download
	*	@returns download count
	*/
	int running_downloads();

	/** Erase a download from the list completely. Not for normal use. always set the status to DOWNLOAD_DELETED instead
	*	@param id ID of the download that should be deleted
	*	@returns success status
	*/
	int remove_download(int id);



	typedef std::vector<download>::iterator iterator;


	std::vector<download> download_list;
	boost::mutex download_mutex;
};

/** Exception class for the download container */
class download_exception {
public:
	download_exception(const char* s) : w(s) {}
	const char* what() { return w.c_str(); }

private:
	mt_string w;
};

#endif // DOWNLOAD_CONTAINER_H_INCLUDED
