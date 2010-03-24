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
#include "helperfunctions.h"
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include <vector>

#ifndef USE_STD_THREAD
#include <boost/thread.hpp>
namespace std {
	using namespace boost;
}
#else
#include <thread>
#endif

#include <sys/stat.h>
#ifdef HAVE_SYSLOG_H
	#include <syslog.h>
#endif
#include <cfgfile/cfgfile.h>
#include "../dl/download.h"
#include "../dl/download_container.h"
#include "../global.h"
using namespace std;

namespace {
	// anonymous namespace to make it file-global
	mutex logfile_mutex;
}

int string_to_int(const std::string str) {
	stringstream ss;
	ss << str;
	int i;
	ss >> i;
	return i;
}

const std::string& trim_string(std::string &str) {
	while(str.length() > 0 && isspace(str[0])) {
		str.erase(str.begin());
	}
	while(str.length() > 0 && isspace(*(str.end() - 1))) {
		str.erase(str.end() -1);
	}
	return str;
}

bool validate_url(std::string &url) {
	bool valid = true;
	// needed for security reason - so the dlist file can't be corrupted
	if(url.find('|') != std::string::npos) {
		valid = false;
	}
	if(url.find("http://") != 0 && url.find("ftp://") != 0 && url.find("https://") != 0) {
		valid = false;
	}
	size_t pos = url.find('/') + 2;
	if(url.find('.', pos +1) == std::string::npos) {
		valid = false;
	}
	return valid;
}

void log_string(const std::string logstr, int level) {

	std::string desiredLogLevel = global_config.get_cfg_value("log_level");
	int desiredLogLevelInt = LOG_DEBUG;

	time_t rawtime;
	time(&rawtime);
	std::string log_date = ctime(&rawtime);
	log_date.erase(log_date.length() - 1);

	if(desiredLogLevel == "OFF") {
		return;
	} else if (desiredLogLevel == "SEVERE") {
		desiredLogLevelInt = LOG_ERR;
	} else if (desiredLogLevel == "WARNING") {
		desiredLogLevelInt = LOG_WARNING;
	} else if (desiredLogLevel == "DEBUG") {
		desiredLogLevelInt = LOG_DEBUG;
	}

	if(desiredLogLevelInt < level) {
		return;
	}

	std::string log_procedure = global_config.get_cfg_value("log_procedure");
	if(log_procedure.empty()) {
		log_procedure = "syslog";
	}

	std::stringstream to_log;
	std::stringstream to_syslog;
	to_log << '[' << log_date << "] ";
	if(level == LOG_ERR) {
		to_log << "SEVERE: ";
		to_syslog << "SEVERE: ";
	} else if(level == LOG_WARNING) {
		to_log << "WARNING: ";
		to_syslog << "WARNING: ";
	} else if(level == LOG_DEBUG) {
		to_log << "DEBUG: ";
		to_syslog << "DEBUG: ";
	}

	to_log << logstr << '\n';
	to_syslog << logstr;

	lock_guard<mutex> lock(logfile_mutex);
	if(log_procedure == "stdout") {
		cout << to_log.str() << flush;
	} else if(log_procedure == "stderr") {
		cerr << to_log.str() << flush;
	} else if(log_procedure == "syslog") {
		#ifdef HAVE_SYSLOG_H
		openlog("DownloadDaemon", LOG_PID, LOG_DAEMON);
		syslog(level, to_syslog.str().c_str(), 0);
		closelog();
		#else
		cerr << "DownloadDaemon compiled without syslog support! Either recompile with syslog support or change the log_procedure!";
		#endif
	}
}


void replace_all(std::string& searchIn, std::string searchFor, std::string ReplaceWith) {
	size_t old_pos = 0;
	size_t new_pos;
	while((new_pos = searchIn.find(searchFor, old_pos)) != std::string::npos) {
		old_pos = new_pos + 2;
		searchIn.replace(new_pos, searchFor.length(), ReplaceWith);
	}
}

std::string long_to_string(long i) {
	stringstream ss;
	ss << i;
	std::string ret;
	ss >> ret;
	return ret;

}

std::string int_to_string(int i) {
	stringstream ss;
	ss << i;
	std::string ret;
	ss >> ret;
	return ret;

}

long string_to_long(std::string str) {
	stringstream ss;
	ss << str;
	long ret;
	ss >> ret;
	return ret;
}

bool variable_is_valid(std::string &variable) {
	trim_string(variable);
	std::string possible_vars = ",enable_resume,enable_reconnect,downloading_active,download_timing_start,download_timing_end,download_folder,"
						   "simultaneous_downloads,log_level,log_procedure,mgmt_max_connections,mgmt_port,mgmt_password,mgmt_accept_enc,max_dl_speed,"
						   "bind_addr,dlist_file,auth_fail_wait,write_error_wait,plugin_fail_wait,connection_lost_wait,refuse_existing_links,overwrite_files,"
						   "recursive_ftp_download,assume_proxys_online,proxy_list,enable_pkg_extractor,pkg_extractor_passwords,download_to_subdirs,";
	size_t pos;
	if((pos = possible_vars.find(variable)) != std::string::npos) {
		if(possible_vars[pos - 1] == ',' && possible_vars[pos + variable.length()] == ',') {
			return true;
		}
	}
	return false;
}

bool proceed_variable(const std::string &variable, std::string value) {
	if(variable == "download_folder") {
		correct_path(value);
		std::string pd = program_root + "/plugins/";
		correct_path(pd);
		std::string rcp = program_root + "/reconnect/";
		correct_path(rcp);
		if(value == pd || value == rcp || value.find("/etc") == 0) {
			return false;
		}
	}

	return true;
}

bool router_variable_is_valid(std::string &variable) {
	trim_string(variable);
	std::string possible_vars = ",reconnect_policy,router_ip,router_username,router_password,";
	size_t pos;
	if((pos = possible_vars.find(variable)) != std::string::npos) {
		if(possible_vars[pos - 1] == ',' && possible_vars[pos + variable.length()] == ',') {
			return true;
		}
	}
	return false;
}

void correct_path(std::string &path) {
	trim_string(path);
	if(path.length() == 0) {
		path = program_root;
		return;
	}

	if(path[0] == '~') {
		path.replace(0, 1, "$HOME");
	}
	substitute_env_vars(path);
	if(path[0] != '/' && path[0] != '\\' && path.find(":\\") == std::string::npos) {
		path.insert(0, program_root);
	}

	char* real_path = realpath(path.c_str(), 0);
	if(real_path != 0) {
		path = real_path;
		free(real_path);
	}

	// remove slashes at the end
	while(*(path.end() - 1) == '/' || *(path.end() - 1) == '\\') {
		path.erase(path.end() - 1);
	}

    std::string::iterator it = path.begin();
#ifdef __CYGWIN__
    ++it; // required to make network-paths possible that start with \\ or //
#endif // __CYGWIN__

	for(; it != path.end(); ++it) {
		if((*it == '/' || *it == '\\') && (*(it + 1) == '/' || *(it + 1) == '\\')) {
			path.erase(it);
		}
	}
}

std::string get_env_var(const std::string &var) {
	std::string result(getenv(var.c_str()));
	return result;
}

void substitute_env_vars(std::string &str) {
	size_t pos = 0, old_pos = 0;
	string var_to_replace;
	string result;
	while((pos = str.find('$', old_pos)) != string::npos) {
		var_to_replace = str.substr(pos + 1, str.find_first_of("$/\\ \n\r\t\0", pos + 1) - pos - 1);
		result = get_env_var(var_to_replace);
		str.replace(pos, var_to_replace.length() + 1, result);
		old_pos = pos + 1;

	}
}

bool mkdir_recursive(std::string dir) {
	size_t len = dir.length();
	if(dir[len -1] == '/') {
		dir[len - 1] = 0;
	}

	if(dir[len - 1] != '/'){
		dir.push_back('/');
	}

	string tmp;
	struct stat st;
	for(size_t index = 0; index < len; ++index) {
		if(dir[index] == '/') {
			tmp = dir.substr(0, dir.find('/', index + 1));
			if(stat(tmp.c_str(), &st) == 0) continue;
			if(mkdir(tmp.c_str(), 0777) != 0) {
				return false;
			}
		}
	}
	return true;
}

std::string filename_from_url(const std::string &url) {
	std::string fn;
	if(url.find("/") != std::string::npos) {
		fn = url.substr(url.find_last_of("/\\"));
		fn = fn.substr(1, fn.find('?') - 1);
	}
	make_valid_filename(fn);
	return fn;
}

/** Functor to compare 2 chars case-insensitive, used as lexicographical_compare callback */
struct lt_nocase : public std::binary_function<char, char, bool> {
	bool operator()(char x, char y) const {
		return toupper(static_cast<unsigned char>(x)) < toupper(static_cast<unsigned char>(y));
	}
};


bool CompareNoCase( const std::string& s1, const std::string& s2 ) {
	return std::lexicographical_compare( s1.begin(), s1.end(), s2.begin(), s2.end(), lt_nocase());
}

void make_valid_filename(std::string &fn) {
	replace_all(fn, "\r\n", "");
	replace_all(fn, "\r", "");
	replace_all(fn, "\n", "");
	replace_all(fn, "/", "");
	replace_all(fn, "\\", "");
	replace_all(fn, ":", "");
	replace_all(fn, "*", "");
	replace_all(fn, "?", "");
	replace_all(fn, "\"", "");
	replace_all(fn, "<", "");
	replace_all(fn, ">", "");
	replace_all(fn, "|", "");
	replace_all(fn, "@", "");
	replace_all(fn, "'", "");
	replace_all(fn, "\"", "");
	replace_all(fn, ";", "");
}

std::string ascii_hex_to_bin(std::string ascii_hex) {
	std::string result;
	for(size_t i = 0; i < ascii_hex.size(); i = i + 2) {
		char hex[5] = "0x00";
		hex[2] = ascii_hex[i];
		hex[3] = ascii_hex[i + 1];
		result.push_back(strtol(hex, NULL, 16));
	}
	return result;
}
