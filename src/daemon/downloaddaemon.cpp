/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "../lib/cfgfile/cfgfile.h"
#include "mgmt/mgmt_thread.h"
#include "dl/download.h"
#include "dl/download_thread.h"
#include "mgmt/global_management.h"
#include "tools/helperfunctions.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <climits>
#include <cstdlib>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <pwd.h>

#define DAEMON_USER "downloadd"

using namespace std;

// GLOBAL VARIABLE DECLARATION:
// The downloadcontainer is just needed everywhere in the program, so let's make it global
download_container global_download_list;
// configuration variables are also used a lot, so global too
cfgfile global_config;
cfgfile global_router_config;
cfgfile global_premium_config;
// same goes for the program root, which is needed for a lot of path-calculation
std::string program_root;
// the environment variables are also needed a lot for path calculations
char** env_vars;

int main(int argc, char* argv[], char* env[]) {
	env_vars = env;
	// Drop user if there is one, and we were run as root
	bool check_home_for_cfg = true;
	if (getuid() == 0 || geteuid() == 0) {
		struct passwd *pw = getpwnam(DAEMON_USER /* "downloadd" */);
		if ( pw ) {
			setuid(pw->pw_uid);
			setgid(pw->pw_gid);
			check_home_for_cfg = false;
		} else {
			std::cerr << "Never run DownloadDaemon as root!" << endl;
			std::cerr << "In order to run DownloadDaemon, please execute these commands as root:" << endl;
			std::cerr << "   addgroup --system " DAEMON_USER << endl;
			std::cerr << "   adduser --system --ingroup " DAEMON_USER " --home /etc/downloaddaemon " DAEMON_USER << endl;
			std::cerr << "   chown -R " DAEMON_USER ":" DAEMON_USER " /etc/downloaddaemon" << endl;
			std::cerr << "then rerun DownloadDaemon." << endl;
			exit(0);
		}
	}
	int lfp = open("/var/lock/downloadd.lock", O_RDWR | O_CREAT, 0640);
	if(lfp < 0) {
		std::cerr << "Unable to create lock-file /var/lock/downloadd.lock" << endl;
		exit(1);
	}
	if(lockf(lfp, F_TLOCK, 0) < 0) {
		std::cerr << "DownloadDaemon is already running. Exiting this instance" << endl;
		exit(0);
	}
	std::stringstream pid;
	pid << getpid();
	write(lfp, pid.str().c_str(), pid.str().length());

	for(int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if(arg == "--help" || arg == "-h") {
			cout << "Usage: downloaddaemon [options]" << endl << endl;
			cout << "Options:" << endl;
			cout << "   -d, --daemon\t\tStart DownloadDaemon in Background" << endl;

			return 0;
		}
		if(arg == "-d" || arg == "--daemon") {
			int j = fork();
			if (j < 0) return 1; /* fork error */
			if (j > 0) return 0; /* parent exits */
			/* child (daemon) continues */
			setsid();
		}
	}

	struct stat st;

	// getting working dir
	std::string argv0 = argv[0];
	program_root = argv[0];
	if(argv0[0] != '/' && argv0.find('/') != string::npos) {
		// Relative path.. sux, but is okay.
		char* c_old_wd = getcwd(0, 0);
		std::string wd = c_old_wd;
		free(c_old_wd);
		wd += '/';
		wd += argv0;
		//
		char* real_path = realpath(wd.c_str(), 0);
		if(real_path == 0) {
			cerr << "Unable to locate executable!" << endl;
			exit(-1);
		}
		program_root = real_path;
		free(real_path);
	} else if(argv0.find('/') == string::npos && !argv0.empty()) {
		// It's in $PATH... let's go!
		std::string env_path(get_env_var("PATH"));

		std::string curr_path;
		size_t curr_pos = 0, last_pos = 0;
		bool found = false;

		while((curr_pos = env_path.find_first_of(":", curr_pos)) != string::npos) {
			curr_path = env_path.substr(last_pos, curr_pos -  last_pos);
			curr_path += '/';
			curr_path += argv[0];
			if(stat(curr_path.c_str(), &st) == 0) {
				found = true;
				break;
			}

			last_pos = ++curr_pos;
		}

		if(!found) {
			// search the last folder of $PATH which is not included in the while loop
			curr_path = env_path.substr(last_pos, curr_pos -  last_pos);
			curr_path += '/';
			curr_path += argv[0];
			if(stat(curr_path.c_str(), &st) == 0) {
				found = true;
			}
		}

		if(found) {
			// successfully located the file..
			// resolve symlinks, etc
			char* real_path = realpath(curr_path.c_str(), 0);
			if(real_path == NULL) {
				cerr << "Unable to locate executable!" << endl;
				exit(-1);
			} else {
				program_root = real_path;
				free(real_path);
			}
		} else {
			cerr << "Unable to locate executable!" << endl;
			exit(-1);
		}
	} else if(argv0.empty()) {

	}

	// else: it's an absolute path.. nothing to do - perfect!
	program_root = program_root.substr(0, program_root.find_last_of('/'));
	program_root = program_root.substr(0, program_root.find_last_of('/'));
	program_root.append("/share/downloaddaemon/");
	if(stat(program_root.c_str(), &st) != 0) {
		cerr << "Unable to locate program data (should be in bindir/../share/downloaddaemon)" << endl;
		cerr << "We were looking in: " << program_root << endl;
		exit(-1);
	}
	chdir(program_root.c_str());

	string home_dd_dir(get_env_var("HOME"));
	home_dd_dir += "/.downloaddaemon/";
	string dd_conf_path("/etc/downloaddaemon/downloaddaemon.conf");
	string premium_conf_path("/etc/downloaddaemon/premium_accounts.conf");
	string router_conf_path("/etc/downloaddaemon/routerinfo.conf");
	if(check_home_for_cfg && stat(string(home_dd_dir + "downloaddaemon.conf").c_str(), &st) == 0) {
		dd_conf_path = home_dd_dir + "downloaddaemon.conf";
	}
	if(check_home_for_cfg && stat(string(home_dd_dir + "premium_accounts.conf").c_str(), &st) == 0) {
		premium_conf_path = home_dd_dir + "premium_accounts.conf";
	}
	if(check_home_for_cfg && stat(string(home_dd_dir + "routerinfo.conf").c_str(), &st) == 0) {
		router_conf_path = home_dd_dir + "routerinfo.conf";
	}

	// check again - will fail if the conf file does not exist at all
	if(stat(dd_conf_path.c_str(), &st) != 0) {
		cerr << "Could not locate configuration file!" << endl;
		exit(-1);
	}

	global_config.open_cfg_file(dd_conf_path.c_str(), true);
	global_router_config.open_cfg_file(router_conf_path.c_str(), true);
	global_premium_config.open_cfg_file(premium_conf_path.c_str(), true);
	if(!global_config) {
		cerr << "Unable to open config file... exiting" << endl;
		exit(-1);
	}
	if(!global_router_config) {
		cerr << "Unable to open router config file" << endl;
	}
	if(!global_premium_config) {
		cerr << "Unable to open premium account config file" << endl;
	}

	std::string dlist_fn = global_config.get_cfg_value("dlist_file");
	correct_path(dlist_fn);

	// set the daemons umask
	std::stringstream umask_ss;
	umask_ss << std::oct;
	umask_ss << global_config.get_cfg_value("daemon_umask");
	int umask_i = 0;
	umask_ss >> umask_i;
	umask(umask_i);

	global_download_list.from_file(dlist_fn.c_str());

	// Create the needed folders
	{
		string dl_folder = global_config.get_cfg_value("download_folder");
		string log_file = global_config.get_cfg_value("log_file");
		string dlist = global_config.get_cfg_value("dlist_file");
		correct_path(dl_folder);
		mkdir_recursive(dl_folder);
		if(log_file != "stdout" && log_file != "stderr") {
			correct_path(log_file);
			log_file = log_file.substr(0, log_file.find_last_of('/'));
			mkdir_recursive(log_file);
		}
		correct_path(dlist);
		dlist = dlist.substr(0, dlist.find_last_of('/'));
		mkdir_recursive(dlist);
	}

	log_string("DownloadDaemon started successfully", LOG_DEBUG);

	{
		// putting it in it's own scope will detach the thread right after creation
		// older boost::thread versions don't have the detach() method but they detach() in the destructor
		boost::thread mgmt_thread(mgmt_thread_main);
	}

	// tick download counters, start new downloads, etc each second
	while(true) {
		do_once_per_second();
		sleep(1);
	}
}
