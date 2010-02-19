/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DOWNLOADC_H
#define DOWNLOADC_H

#include <vector>
#include <string>
#include <boost/thread.hpp>
#include <netpptk/netpptk.h>


/** Download Struct, defines one single Download */
struct download{
	int id;
	std::string date;
	std::string title;
	std::string url;
	std::string status;
	uint64_t downloaded;
	uint64_t size;
	int wait;
	std::string error;
	int speed;
};

/** Package Struct, defines a Package containing an ID and Downloads */
struct package{
	int id;
	std::string name;
	std::vector<download> dls;
};


/** DownloadClient Class, makes communication with DownloadDaemon easier */
class downloadc{
	public:

		/** Defaultconstructor */
		downloadc();

		/** Destructor */
		~downloadc();

		/** Connects to a DownloadDaemon, successful or not shown by exception
		*	@param host ip where the daemon runs
		*	@param port port where the daemon runs
		*	@param pass password of daemon
		*	@param encrypt force encryption for sending password or not
		*/
		void connect(std::string host = "127.0.0.1", int port = 56789, std::string pass = "", bool encrypt = false);

		/** Returns a list of all downloads
		*	@returns list of downloads
		*/
		std::vector<package> get_list();

		/** Adds a download, successful or not shown by exception
		*	@param package package where the download should be in
		*	@param url URL of download
		*	@param title title of download
		*/
		void add_download(int package, std::string url, std::string title); // if package doesn't exist, create it with add_package!

		/* missing methods.... (see comment below) */

		/** Adds a package, successful or not shown by exception
		*	@param name optional name of the package
		*/
		void add_package(std::string name = "");

		/*
		downloads:
		delete_download
		stop
		up
		down
		activate
		deactivate

		package:
		add
		delete
		up
		down
		exists

		var:
		get
		set

		file:
		del
		getfile (not implemented)
		getpath
		getsize

		router:
		list
		setmodel
		set
		get

		premium:
		list
		set
		get
		*/

	private:

		tkSock *mysock;
		boost::mutex mx;

		void check_connection();
};

#endif // DOWNLOADC_H
