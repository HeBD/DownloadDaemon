#include "helperfunctions.h"
#include <sstream>
#include <string>
#include <boost/regex.hpp>
#include <iostream>
#include <fstream>
#include <ctime>
#include "../../lib/cfgfile.h"

using namespace std;
extern cfgfile global_config;

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
	boost::regex rx("^(?#Protocol)(?:(?:ht|f)tp(?:s?)\\:\\/\\/|~/|/)?(?#Username:Password)(?:\\w+:\\w+@)?(?#Subdomains)(?:(?:[-\\w]+\\.)"
					"+(?#TopLevel Domains)(?:com|org|net|gov|mil|biz|info|mobi|name|aero|jobs|museum|travel|[a-z]{2}))(?#Port)(?::[\\d]"
					"{1,5})?(?#Directories)(?:(?:(?:/(?:[-\\w~!$+|.,=]|%[a-f\\d]{2})+)+|/)+|\\?|#)?(?#Query)(?:(?:\\?(?:[-\\w~!$+|.,*:]"
					"|%[a-f\\d{2}])+=(?:[-\\w~!$+|.,*:=]|%[a-f\\d]{2})*)(?:&(?:[-\\w~!$+|.,*:]|%[a-f\\d{2}])+=(?:[-\\w~!$+|.,*:=]|%[a-f\\d]"
					"{2})*)*)*(?#Anchor)(?:#(?:[-\\w~!$+|.,*:=]|%[a-f\\d]{2})*)?$");
	return regex_match(url, rx);
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

std::string int_to_string(int i) {
	std::stringstream ss;
	ss << i;
	std::string ret;
	ss >> ret;
	return ret;

}
