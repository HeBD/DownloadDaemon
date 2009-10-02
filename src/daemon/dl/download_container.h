/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

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
	*	@returns LIST_ID if it's already the top download, LIST_PERMISSION if the function fails because the dlist can't be written, LIST_SUCCESS on success
	*/
	int move_up(int id);

	/** Moves a download downwards
	*	@param id download ID to move down
	*	@returns LIST_ID if it's already the top download, LIST_PERMISSION if the function fails because the dlist can't be written, LIST_SUCCESS on success
	*/
	int move_down(int id);

	/** Activates an inactive download
	*	@param id download ID to activate
	*	@returns LIST_ID if an invalid ID is given, LIST_PROPERTY if the download is already active, LIST_PERMISSION if the dlist file can't be written, LIST_SUCCESS
	*/
	int activate(int id);

	/** Deactivates a download
	*	@param id download ID to deactivate
	*	@returns LIST_ID if an invalid ID is given, LIST_PROPERTY if the download is already inactive, LIST_PERMISSION if the dlist file can't be written, LIST_SUCCESS
	*/
	int deactivate(int id);

	/** Gets the next downloadable item in the global download list (filters stuff like inactives, wrong time, etc)
 	*	@returns ID of the next download that can be downloaded or LIST_ID if there is none, LIST_SUCCESS
 	*/
	int get_next_downloadable(bool do_lock = true);

	/** Adds a new download to the list
	*	@param dl Download object to add
	*	@returns LIST_SUCCESS, LIST_PERMISSION
	*/
	int add_download(const download &dl);


	/** Functions to set download element variables
	*	@param id ID of the download to set a variable for
	*	@param prop Property to set. depending on the function, this can be one of the above enums (property, pointer_property, string_property)
	*	@param value Value to set the variable to
	*	@returns LIST_SUCCESS, LIST_PERMISSION, LIST_ID, LIST_PROPERTY
	*/
	int set_string_property(int id, string_property prop, std::string value);
	int set_int_property(int id, property prop, double value);
	int set_pointer_property(int id, pointer_property prop, void* value);

	/** Functions to get download element variables. pointer_property will return a 0-pointer if it fails, int_property will return LIST_PROPERTY and string_property will throw a download_exception
	*	@param id ID of the download to get a variable from
	*	@param prop Property to get
	*/
	std::string get_string_property(int id, string_property prop);
	void* get_pointer_property(int id, pointer_property prop);
	double get_int_property(int id, property prop);

	/** Prepares a download (calls the plugin, etc)
	*	@param dl Download id to prepare
	*	@param poutp plugin_output structure, will be filled in by the plugin
	*	@returns LIST_ID, LIST_SUCCESS
	*/
	int prepare_download(int dl, plugin_output &poutp);

	/** Returns info about a plugin
	*	@param dl Download toget info from
	*	@returns the info
	*/
	plugin_output get_hostinfo(int dl);

	/** strip the host from the URL
	*	@param dl Download from which to get the host
	*	@returns the hostname
	*/
	std::string get_host(int dl);

	/** Every download with status DOWNLOAD_WAITING and wait seconds > 0 will decrease wait seconds by one. If 0 is reached, the status will be set to DOWNLOAD_PENDING
	*/
	void decrease_waits();

	/** Removes downloads with a DOWNLOAD_DELETED status from the list, if they can be deleted without danger
	*/
	void purge_deleted();

	/** Creates the list for the DL LIST command
	*	@returns the list
	*/
	std::string create_client_list();

	/** Gets the lowest unused ID that should be used for the next download
	*	@returns ID
	*/
	int get_next_id();

	/** Stops a download and sets its status
	*	@param id Download to stop
	*	@returns LIST_SUCCESS, LIST_ID, LIST_PERMISSION
	*/
	int stop_download(int id);

	/** Checks if a reconnect is currently needed
	*	@returns true if yes
	*/
	bool reconnect_needed();

	static void do_reconnect(download_container *dlist);

	std::string list_file;

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
	bool is_reconnecting;
};

/** Exception class for the download container */
class download_exception {
public:
	download_exception(const char* s) : w(s) {}
	const char* what() { return w.c_str(); }

private:
	std::string w;
};

#endif // DOWNLOAD_CONTAINER_H_INCLUDED
