/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include"cfgfile.h"
#include<fstream>
#include<string>
#include<vector>
#include<boost/thread.hpp>
using namespace std;

cfgfile::cfgfile()
	: comment_token("#"), is_writeable(false), eqtoken('=') {
}

cfgfile::cfgfile(std::string &fp, bool open_writeable)
	: filepath(fp), comment_token("#"), is_writeable(open_writeable), eqtoken('=') {
	open_cfg_file(filepath, is_writeable);
	comment_token = "#";
}

cfgfile::cfgfile(std::string &fp, std::string &comm_token, char eq_token, bool open_writeable)
	: filepath(fp), comment_token(comm_token), is_writeable(open_writeable), eqtoken(eq_token) {
	open_cfg_file(filepath, is_writeable);
}

cfgfile::~cfgfile() {
	file.close();
}

void cfgfile::open_cfg_file(const std::string &fp, bool open_writeable) {
	file.close();
	filepath = fp;
	if (open_writeable) {
		file.open(filepath.c_str(), fstream::out | fstream::in | fstream::ate);
		is_writeable = true;
	}
	else {
		file.open(filepath.c_str(), fstream::in);
		is_writeable = false;
	}
}

std::string cfgfile::get_cfg_value(const std::string &cfg_identifier) {
	boost::mutex::scoped_lock lock(mx);
	if(!file.is_open() || !file.good()) {
		mx.unlock();
		reload_file();
		mx.lock();
	}
	file.seekg(0);
	std::string buff, identstr, val;
	size_t eqloc;
	while(getline(file, buff) && !file.eof() && file.good()) {
		buff = buff.substr(0, buff.find(comment_token));
		eqloc = buff.find(eqtoken);
		identstr = buff.substr(0, eqloc);
		trim(identstr);
		if(identstr == cfg_identifier) {
			val = buff.substr(eqloc +1);
			trim(val);
			return val;
		}
	}

	return "";
}

bool cfgfile::set_cfg_value(const std::string &cfg_identifier, const std::string &value) {
	mx.lock();
	std::string cfg_value = value;
	trim(cfg_value);
	if(!is_writeable || !file.is_open()) {
		mx.unlock();
		return false;
	}

	file.seekg(0);
	vector<std::string> cfgvec;
	std::string buff, newbuff, identstr, val;
	size_t eqloc;
	bool done = false;

	while(!file.eof() && file.good()) {
		getline(file, buff);
		cfgvec.push_back(buff);
	}

	for(vector<std::string>::iterator it = cfgvec.begin(); it != cfgvec.end(); ++it) {
		newbuff = it->substr(0, it->find(comment_token));
		eqloc = newbuff.find(eqtoken);
		if(eqloc == string::npos) {
			continue;
		}
		identstr = newbuff.substr(0, eqloc);
		trim(identstr);
		if(identstr == cfg_identifier) {
			// to protect from substr-segfaults
			*it += " ";
			*it = it->substr(0, eqloc + 1);
			*it += " ";
			*it += cfg_value;
			done = true;
			break;
		}
	}

	if(!done) {
		cfgvec.push_back(cfg_identifier + " = " + cfg_value);
	}

	// write to file.. need to close/reopen it in order to delete the existing content
	file.close();
	file.open(filepath.c_str(), fstream::in | fstream::out | fstream::trunc);
	for(vector<std::string>::iterator it = cfgvec.begin(); it != cfgvec.end(); ++it) {
		file << *it;
		if(it != cfgvec.end() - 1) {
			file << '\n';
		}
	}

	mx.unlock();
	reload_file();

	return true;
}

std::string cfgfile::get_comment_token() const {
	return comment_token;
}

void cfgfile::set_comment_token(const std::string &token) {
	comment_token = token;
}

void cfgfile::reload_file() {
	mx.lock();
	file.close();

	if(is_writeable) {
		file.open(this->filepath.c_str(), fstream::in | fstream::out | fstream::ate);
	} else {
		file.open(this->filepath.c_str(), fstream::in);
	}
	mx.unlock();
}

void cfgfile::close_cfg_file() {
	mx.lock();
	file.close();
	mx.unlock();
}

inline bool cfgfile::writeable() const {
	return is_writeable;
}

inline std::string cfgfile::get_filepath() const {
	return filepath;
}

bool cfgfile::list_config(std::string& resultstr) {
	mx.lock();
	if(!file.is_open()) {
		mx.unlock();
		return false;
	}
	std::string buff;
	resultstr = "";
	while(!file.eof() && file.good()) {
		getline(file, buff);
		trim(buff);
		if(buff.empty()) {
			continue;
		} else {
			if(buff.find(comment_token, 0) != std::string::npos) {
				buff = buff.substr(0, buff.find(comment_token, 0));
			}
			resultstr += buff;
			resultstr += '\n';
		}
	}
	mx.unlock();
	return true;

}

void cfgfile::trim(std::string &str) const {
	while(str.length() > 0 && isspace(str[0])) {
		str.erase(str.begin());
	}
	while(str.length() > 0 && isspace(*(str.end() - 1))) {
		str.erase(str.end() -1);
	}
}
