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

enum file_delete { del_file, dont_delete, dont_know };

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


		// target DL
		/** Returns a list of all downloads
		*	@returns list of downloads
		*/
		std::vector<package> get_list();

		/** Adds a download, successful or not shown by exception
		*	@param package package where the download should be in
		*	@param url URL of download
		*	@param title title of download
		*/
		void add_download(int package, std::string url, std::string title="");

		/** Deletes a download, successful or not shown by exception
		*	@param id ID of the download
		*	@param delete_file shows how to handle downloaded files of the download to be deleted
		*/
		void delete_download(int id, file_delete = dont_know);

		/** Stops a download, successful or not shown by exception
		*	@param id ID of the download to be stopped
		*/
		void stop_download(int id);

		/** Sets priority of a download up, successful or not shown by exception
		*	@param id ID of the download
		*/
		void priority_up(int id);

		/** Sets priority of a download down, successful or not shown by exception
		*	@param id ID of the download
		*/
		void priority_down(int id);

		/** Activates a download, successful or not shown by exception
		*	@param id ID of the download
		*/
		void activate_download(int id);

		/** Deactivates a download, successful or not shown by exception
		*	@param id ID of the download
		*/
		void deactivate_download(int id);


		// target PKG
		/** Adds a package, successful or not shown by exception
		*	@param name optional name of the package
		*/
		void add_package(std::string name = "");

		/** Deletes a package
		*	@param id ID of the package
		*/
		void delete_package(int id);

		/** Sets priority of a package up
		*	@param id ID of the package
		*/
		void package_priority_up(int id);

		/** Sets priority of a package up
		*	@param id ID of the package
		*/
		void package_priority_down(int id);

		/** Checks the existance of a package
		*	@param id ID of the package
		*	@param true if package exists
		*/
		bool package_exists(int id);


		// target VAR
		/** Setter for variables, successful or not shown by exception
		*	@param var Name of the variable
		*	@param value Value to be set
		*	@param value Optional old value, needed if you set variable mgmt_password
		*/
		void set_var(std::string var, std::string value, std::string old_value = "");

		/** Getter for variables
		*	@param var variable which value should be returned
		*	@returns value or empty string
		*/
		std::string get_var(std::string var);


		// target FILE
		/** Deletes a downloaded file, successful or not shown by exception
		*	@param id ID of the file to be deleted
		*/
		void delete_file(int id);

		/** Returns the file as binary data, NOT IMPLEMENTED BY DAEMON, DO NOT USE!
		*	@param id ID of the file to be sent
		*/
		void get_file(int id);

		/** Returns local path of a file
		*	@param id ID of the file
		*	@returns path or empty string if it fails
		*/
		std::string get_file_path(int id);

		/** Returns size of a file in byte
		*	@param id ID of the file
		*	@returns filesize in byte
		*/
		double get_file_size(int id);


		// target ROUTER
		/** Returns the router list, successful or not shown by exception
		*	@returns router list in a vector
		*/
		std::vector<std::string> get_router_list();

		/** Sets the router model, successful or not shown by exception
		*	@param model model to be set
		*/
		void set_router_model(std::string model);

		/** Setter for router variables, successful or not shown by exception
		*	@param var Name of the variable
		*	@param value Value to be set
		*/
		void set_router_var(std::string var, std::string value);

		/** Getter for router variables
		*	@param var variable which value should be returned
		*	@returns value or empty string
		*/
		std::string get_router_var(std::string var);


		// target PREMIUM
		/** Returns the premium list, successful or not shown by exception
		*	@returns premium list in a vector
		*/
		std::vector<std::string> get_premium_list();

		/** Setter for premium host, user and password, successful or not shown by exception
		*	@param host premiumhost
		*	@param user username
		*	@param password password
		*/
		void set_premium_var(std::string host, std::string user, std::string password);

		/** Getter for premium username
		*	@param host host which the username is for
		*	@returns value or empty string
		*/
		std::string get_premium_var(std::string host);


	private:

		tkSock *mysock;
		boost::mutex mx;

		void check_connection();
		void check_error_code(std::string check_me);
};

#endif // DOWNLOADC_H
