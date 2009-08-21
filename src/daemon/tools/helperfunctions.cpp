#include "helperfunctions.h"
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include <vector>
#include <boost/thread.hpp>
#include "../../lib/cfgfile/cfgfile.h"
#include "../dl/download.h"
#include "../dl/download_container.h"
using namespace std;

extern cfgfile global_config;
extern string program_root;
extern download_container global_download_list;
extern string program_root;

namespace {
	// anonymous namespace to make it file-global
	boost::mutex logfile_mutex;
}

int string_to_int(const std::string str) {
	std::stringstream ss;
	ss << str;
	int i;
	ss >> i;
	return i;
}

void trim_string(std::string &str) {
	while(str.length() > 0 && isspace(str[0])) {
		str.erase(str.begin());
	}
	while(str.length() > 0 && isspace(*(str.end() - 1))) {
		str.erase(str.end() -1);
	}
}

bool validate_url(std::string &url) {
	bool valid = true;
	if(url.find('|') != string::npos) {
		valid = false;
	}
	if(url.find("http://") != 0 && url.find("ftp://") != 0) {
		valid = false;
	}
	size_t pos = url.find('/') + 2;
	if(url.find('.', pos +1) == string::npos) {
		valid = false;
	}
	return valid;
}

void log_string(const std::string logstr, LOG_LEVEL level) {
	std::string desiredLogLevel = global_config.get_cfg_value("log_level");
	LOG_LEVEL desiredLogLevelInt = LOG_WARNING;
	std::string desiredLogFile = global_config.get_cfg_value("log_file");

	time_t rawtime;
	time(&rawtime);
	string log_date = ctime(&rawtime);
	log_date.erase(log_date.length() - 1);

	if(desiredLogLevel == "OFF") {
		desiredLogLevelInt = LOG_OFF;
	} else if (desiredLogLevel == "SEVERE") {
		desiredLogLevelInt = LOG_SEVERE;
	} else if (desiredLogLevel == "WARNING") {
		desiredLogLevelInt = LOG_WARNING;
	} else if (desiredLogLevel == "DEBUG") {
		desiredLogLevelInt = LOG_DEBUG;
	}

	if(desiredLogLevelInt < level) {
		return;
	}

	boost::mutex::scoped_lock(logfile_mutex);
	if(desiredLogFile == "stdout" || desiredLogFile == "") {
		cout << '[' << log_date << "] " << logstr << '\n' << flush;
	} else if(desiredLogFile == "stderr") {
		cerr << '[' << log_date << "] " << logstr << '\n' << flush;
	} else {
		ofstream ofs(desiredLogFile.c_str(), ios::app);
		ofs << '[' << log_date << "] " << logstr << '\n' << flush;
		ofs.close();
	}
}


void replace_all(std::string& searchIn, std::string searchFor, std::string ReplaceWith) {
	size_t old_pos = 0;
	size_t new_pos;
	while((new_pos = searchIn.find(searchFor, old_pos)) != string::npos) {
		old_pos = new_pos + 2;
		searchIn.replace(new_pos, searchFor.length(), ReplaceWith);
	}
}

std::string long_to_string(long i) {
	std::stringstream ss;
	ss << i;
	std::string ret;
	ss >> ret;
	return ret;

}

std::string int_to_string(int i) {
	std::stringstream ss;
	ss << i;
	std::string ret;
	ss >> ret;
	return ret;

}

long string_to_long(string str) {
	std::stringstream ss;
	ss << str;
	long ret;
	ss >> ret;
	return ret;
}

bool variable_is_valid(std::string &variable) {
	trim_string(variable);
	string possible_vars = ",enable_resume,enable_reconnect,downloading_active,download_timing_start,download_timing_end,download_folder,"
                           "simultaneous_downloads, log_level,log_file,mgmt_max_connections,mgmt_port,mgmt_password,plugin_dir,"
                           "dlist_file,";
	size_t pos;
	if((pos = possible_vars.find(variable)) != string::npos) {
		if(possible_vars[pos - 1] == ',' && possible_vars[pos + variable.length()] == ',') {
			return true;
		}
	}
	return false;
}

bool router_variable_is_valid(std::string &variable) {
    trim_string(variable);
    string possible_vars = ",reconnect_policy,router_ip,router_username,router_password,";
    size_t pos;
	if((pos = possible_vars.find(variable)) != string::npos) {
		if(possible_vars[pos - 1] == ',' && possible_vars[pos + variable.length()] == ',') {
			return true;
		}
	}
	return false;
}

void correct_path(std::string &path) {
    trim_string(path);
    if(path[0] != '/') {
        path.insert(0, program_root);
    }
}
