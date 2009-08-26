#include"cfgfile.h"
#include<fstream>
#include<string>
#include<vector>
#include<boost/thread.hpp>
using namespace std;

cfgfile::cfgfile()
	: comment_token("#"), is_writeable(false), eqtoken('=') {
}

cfgfile::cfgfile(mt_string &filepath, bool is_writeable)
	: filepath(filepath), comment_token("#"), is_writeable(is_writeable), eqtoken('=') {
	open_cfg_file(filepath, is_writeable);
	comment_token = "#";
}

cfgfile::cfgfile(mt_string &filepath, mt_string &comment_token, char eqtoken, bool is_writeable)
	: filepath(filepath), comment_token(comment_token), is_writeable(is_writeable), eqtoken(eqtoken) {
	open_cfg_file(filepath, is_writeable);
}

cfgfile::~cfgfile() {
	file.close();
}

void cfgfile::open_cfg_file(const mt_string &filepath, bool writeable) {
	file.close();
	if (writeable) {
		file.open(filepath.c_str(), fstream::out | fstream::in | fstream::ate);
		is_writeable = true;
		this->filepath = filepath;
	}
	else {
		file.open(filepath.c_str(), fstream::in);
		is_writeable = false;
		this->filepath = filepath;
	}
}

mt_string cfgfile::get_cfg_value(const mt_string &cfg_identifier) {
	mx.lock();
	if(!file.is_open()) {
		mx.unlock();
		return "";
	}
	fstream tmpfile(filepath.c_str(), fstream::in); // second fstream to make const possible
	mt_string buff, identstr, val;
	size_t eqloc;
	while(!tmpfile.eof() && tmpfile.good()) {
		getline(tmpfile, buff);
		buff = buff.substr(0, buff.find(comment_token));
		eqloc = buff.find(eqtoken);
		identstr = buff.substr(0, eqloc);
		trim(identstr);
		if(identstr == cfg_identifier) {
			val = buff.substr(eqloc +1);
			trim(val);
			tmpfile.close();
			mx.unlock();
			return val;
		}
	}
	tmpfile.close();
	mx.unlock();
	return "";
}

bool cfgfile::set_cfg_value(const mt_string &cfg_identifier, const mt_string &cfg_value) {
	mx.lock();
	if(!is_writeable || !file.is_open()) {
		mx.unlock();
		return false;
	}

	file.seekg(0);
	vector<mt_string> cfgvec;
	mt_string buff, newbuff, identstr, val;
	size_t eqloc;
	bool done = false;

	while(!file.eof() && file.good()) {
		getline(file, buff);
		cfgvec.push_back(buff);
	}

	for(vector<mt_string>::iterator it = cfgvec.begin(); it != cfgvec.end(); ++it) {
		newbuff = it->substr(0, it->find(comment_token));
		eqloc = newbuff.find(eqtoken);
		identstr = newbuff.substr(0, eqloc);
		trim(identstr);
		if(identstr == cfg_identifier) {
			val = newbuff.substr(eqloc + 1);
			trim(val);
			it->replace(it->find(val, eqloc + 1), val.length(), cfg_value);
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
	for(vector<mt_string>::iterator it = cfgvec.begin(); it != cfgvec.end(); ++it) {
		file << *it << '\n';
	}

	mx.unlock();
	reload_file();

	return true;
}

mt_string cfgfile::get_comment_token() const {
	return comment_token;
}

void cfgfile::set_comment_token(const mt_string &comment_token) {
	this->comment_token = comment_token;
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

inline mt_string cfgfile::get_filepath() const {
	return filepath;
}

bool cfgfile::list_config(mt_string& resultstr) {
	mx.lock();
	if(!file.is_open()) {
		mx.unlock();
		return false;
	}
	mt_string buff;
	resultstr = "";
	while(!file.eof() && file.good()) {
		getline(file, buff);
		trim(buff);
		if(buff.empty()) {
			continue;
		} else {
			if(buff.find(comment_token, 0) != mt_string::npos) {
				buff = buff.substr(0, buff.find(comment_token, 0));
			}
			resultstr += buff;
			resultstr += '\n';
		}
	}
	mx.unlock();
	return true;

}

void cfgfile::trim(mt_string &str) const {
	while(str.length() > 0 && isspace(str[0])) {
		str.erase(str.begin());
	}
	while(str.length() > 0 && isspace(*(str.end() - 1))) {
		str.erase(str.end() -1);
	}
}
