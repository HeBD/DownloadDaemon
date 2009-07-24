#include "helperfunctions.h"
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include <vector>
#include "../../lib/cfgfile.h"
#include "../dl/download.h"
using namespace std;

extern cfgfile global_config;
extern string program_root;
extern std::vector<download> global_download_list;

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
	if(url.find("http://") != 0 || url.find("ftp://") != 0) {
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

	if(desiredLogFile == "stdout" || desiredLogFile == "") {
		cout << '[' << log_date << "] " << logstr << '\n' << flush;
	} else if(desiredLogFile == "stderr") {
		cerr << '[' << log_date << "] " << logstr << '\n' << flush;
	} else {
		ofstream ofs(desiredLogFile.c_str(), ios::app);
		ofs << '[' << log_date << "] " << logstr;
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

bool dump_list_to_file() {
	fstream dlfile(string(program_root + global_config.get_cfg_value("dlist_file")).c_str(), ios::trunc | ios::out);
	if(!dlfile.good()) {
		return false;
	}

	for(vector<download>::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
		dlfile << it->serialize() << '\n';
	}
	dlfile.close();

	return true;
}
