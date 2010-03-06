/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <config.h>
#include <cfgfile/cfgfile.h>
#include "mgmt/mgmt_thread.h"
#include "dl/download.h"
#include "dl/package_container.h"
#include "mgmt/global_management.h"
#include "tools/helperfunctions.h"

#ifndef USE_STD_THREAD
#include <boost/thread.hpp>
namespace std {
	using namespace boost;
}
#else
#include <thread>
#endif

#include <iostream>
#include <vector>
#include <string>
#include <climits>
#include <cstdlib>
#include <sstream>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <pwd.h>

#ifndef HAVE_UINT64_T
	#warning You do not have the type uint64_t. this is pretty bad and you might get problems when you download big files.
	#warning This includes that DownloadDaemon may display wrong download sizes/progress. But downloading should work.. maybe...
#endif

#define DAEMON_USER "downloadd"

using namespace std;

#ifndef DD_CONF_DIR
	#define DD_CONF_DIR "/etc/downloaddaemon/"
#endif

// GLOBAL VARIABLE DECLARATION:
// The downloadcontainer is just needed everywhere in the program, so let's make it global
package_container global_download_list;
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
		if ( pw && setgid(pw->pw_gid) == 0 && setuid(pw->pw_uid) == 0) {
			check_home_for_cfg = false;
		} else {
			std::cerr << "Never run DownloadDaemon as root!" << endl;
			std::cerr << "In order to run DownloadDaemon, please execute these commands as root:" << endl;
			std::cerr << "   addgroup --system " DAEMON_USER << endl;
			std::cerr << "   adduser --system --ingroup " DAEMON_USER " --home " DD_CONF_DIR " " DAEMON_USER << endl;
			std::cerr << "   chown -R " DAEMON_USER ":" DAEMON_USER " " DD_CONF_DIR " /var/downloads" << endl;
			std::cerr << "then rerun DownloadDaemon." << endl;
			exit(0);
		}
	}

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

	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 1;
	int fdlock;
	#ifndef __CYGWIN__
	if((fdlock = open("/tmp/downloadd.lock", O_WRONLY|O_CREAT, 0777)) < 0 || fcntl(fdlock, F_SETLK, &fl) == -1) {
		std::cerr << "DownloadDaemon is already running. Exiting this instance" << endl;
		exit(0);
	}
	// need to chmod seperately because the permissions set in open() are affected by the umask. chmod() isn't
	fchmod(fdlock, 0777);
	#else
	string p_tmp = getenv("PATH");
	p_tmp += ":/bin:.";
	setenv("PATH", p_tmp.c_str(), 1);
	#endif


	struct stat st;

	// getting working dir
	std::string argv0 = argv[0];
	program_root = argv0;

	if(argv0[0] != '/' && argv0[0] != '\\' && (argv0.find('/') != string::npos || argv0.find('\\') != string::npos)) {
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
	} else if(argv0.find('/') == string::npos && argv0.find('\\') == string::npos && !argv0.empty()) {
		// It's in $PATH... let's go!
		std::string env_path(get_env_var("PATH"));

		std::string curr_path;
		size_t curr_pos = 0, last_pos = 0;
		bool found = false;

		while((curr_pos = env_path.find_first_of(":;", curr_pos)) != string::npos) {
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
	program_root = program_root.substr(0, program_root.find_last_of("/\\"));
	program_root = program_root.substr(0, program_root.find_last_of("/\\"));
	program_root.append("/share/downloaddaemon/");

	if(stat(program_root.c_str(), &st) != 0) {
		#ifdef __CYGWIN__
			if(program_root.find("/usr/share/") != string::npos) {
				program_root = "/share/downloaddaemon/";
			}
		#else
			cerr << "Unable to locate program data (should be in bindir/../share/downloaddaemon)" << endl;
			cerr << "We were looking in: " << program_root << endl;
			exit(-1);
		#endif
	}
	chdir(program_root.c_str());

	string dd_conf_path(DD_CONF_DIR "/downloaddaemon.conf");
	string premium_conf_path(DD_CONF_DIR "/premium_accounts.conf");
	string router_conf_path(DD_CONF_DIR "/routerinfo.conf");


	// check again - will fail if the conf file does not exist at all
	if(stat(dd_conf_path.c_str(), &st) != 0) {
		cerr << "Could not locate configuration file!" << endl;
		exit(-1);
	}

	global_config.open_cfg_file(dd_conf_path.c_str(), true);
	global_router_config.open_cfg_file(router_conf_path.c_str(), true);
	global_premium_config.open_cfg_file(premium_conf_path.c_str(), true);
	if(!global_config) {
		uid_t uid = geteuid();
		struct passwd *pw = getpwuid(uid);
		std::string unam = "downloadd";
		if(pw) {
			unam = pw->pw_name;
		}

		cerr << "Unable to open config file!" << endl;
		cerr << "You probably don't have enough permissions to write the configuration file. executing \"chown -R "
			 << unam << ":downloadd " DD_CONF_DIR " /var/downloads\" might help" << endl;
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
		correct_path(dlist);
		dlist = dlist.substr(0, dlist.find_last_of('/'));
		mkdir_recursive(dlist);
	}

	log_string("DownloadDaemon started successfully", LOG_DEBUG);

	{
		// putting it in it's own scope will detach the thread right after creation
		// older boost::thread versions don't have the detach() method but they detach() in the destructor
		thread mgmt_thread(mgmt_thread_main);
		mgmt_thread.detach();
	}

	// tick download counters, start new downloads, etc each second
	thread once_per_sec_thread(do_once_per_second);
	once_per_sec_thread.detach();
	while(true) {
		sleep(1);
		once_per_sec_mutex.unlock();
	}
}
