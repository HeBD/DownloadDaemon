#ifndef DOWNLOAD_CONTAINER_H_INCLUDED
#define DOWNLOAD_CONTAINER_H_INCLUDED

#include "download.h"
#include <string>
#include <vector>
#include <boost/thread.hpp>


extern boost::mutex download_container_mutex;

class download_container {
public:
	class iterator {
	public:
		iterator() {}
		iterator(std::vector<download>::iterator it) { boost::mutex::scoped_lock(download_container_mutex); download_iterator = it; }
		download_container::iterator operator++() { boost::mutex::scoped_lock(download_container_mutex); ++download_iterator; return *this; }
		download_container::iterator operator--() { boost::mutex::scoped_lock(download_container_mutex); --download_iterator; return *this; }
		download_container::iterator operator++(int i) { boost::mutex::scoped_lock(download_container_mutex); download_container::iterator tmp = *this; ++download_iterator; return tmp; }
		download_container::iterator operator--(int i) { boost::mutex::scoped_lock(download_container_mutex); download_container::iterator tmp = *this; --download_iterator; return tmp; }
		download operator*() { boost::mutex::scoped_lock(download_container_mutex); return *download_iterator; }
		download* operator->() { boost::mutex::scoped_lock(download_container_mutex); return &(*download_iterator); }
		download_container::iterator operator+(size_t i) { boost::mutex::scoped_lock(download_container_mutex); download_container::iterator it(*this); it.download_iterator += i; return it; }
		download_container::iterator operator-(size_t i) { boost::mutex::scoped_lock(download_container_mutex); download_container::iterator it(*this); it.download_iterator -= i; return it; }
		download_container::iterator operator+=(size_t i) { boost::mutex::scoped_lock(download_container_mutex); download_iterator += i; return *this; }
		download_container::iterator operator -=(size_t i) { boost::mutex::scoped_lock(download_container_mutex); download_iterator -= i; return *this; }
		bool operator==(iterator it) { boost::mutex::scoped_lock(download_container_mutex); return download_iterator == it.download_iterator; }
		bool operator!=(iterator it) { boost::mutex::scoped_lock(download_container_mutex); return  download_iterator != it.download_iterator; }
		operator std::vector<download>::iterator() { boost::mutex::scoped_lock(download_container_mutex); return download_iterator; }
	private:
		std::vector<download>::iterator download_iterator;
	};
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
	bool from_file(const char* filename);

	/** get an iterator to a download by giving an ID
	*	@param id download ID to search for
	*	@returns Iterator to this id
	*/
	download_container::iterator get_download_by_id(int id);

	/** Iterator to first element
	* @returns first element iterator
	*/
	iterator begin() { return iterator(download_list.begin()); }

	/** Retruns an iterator to the past-end of the list
	* @returns past-end-iterator
	*/
	download_container::iterator end() { return iterator(download_list.end()); }

	/** Check if download list is empty
	*	@returns True if empty
	*/
	bool empty() { return download_list.empty(); }

	/** Adds a download at the end of the list
	*	@param dl Download object to add
	*	@returns true on success
	*/
	bool push_back(download dl);

	/** Erases the last download
	*	@returns true on success
	*/
	bool pop_back();

	/** Erase a download from the list. USE WITH CARE! NOT THREAD-SAFE!
	*	@param it Iterator to the download that should be deleted
	*	@returns true on success
	*/
	bool erase(download_container::iterator it);

	/** Returns the amount of running download
	*	@returns download count
	*/
	int running_downloads();

	/** Returns the total amount of downloads in the list
	*	@returns download count
	*/
	int total_downloads();

	/** Dumps the download list from RAM to the file
	*	@returns true on success
	*/
	bool dump_to_file();

	/** Moves a download upwards
	*	@param id download ID to move up
	*	@returns -1 if it's already the top download, -2 if the function fails because the dlist can't be written, 0 on success
	*/
	int move_up(int id);

	/** Gets the lowest unused ID that should be used for the next download
	*	@returns ID
	*/
	int get_next_id();

	/** Sorts the download list by ID
	*/
	void arrange_by_id();

	/** Gets the next downloadable item in the global download list (filters stuff like inactives, wrong time, etc)
 	*	@returns iterator to the correct download object from the global download list, iterator to end() if nothing can be downloaded
 	*/
	iterator get_next_downloadable();

private:

	mt_string list_file;
	std::vector<download> download_list;
};



#endif // DOWNLOAD_CONTAINER_H_INCLUDED
