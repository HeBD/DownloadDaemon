#include"cfgfile.h"
#include<fstream>
#include<string>
#include<vector>
using namespace std;

cfgfile::cfgfile()
	: comment_token("#"), is_writeable(false), eqtoken('=') {
}

cfgfile::cfgfile(const string &filepath, bool is_writeable = false)
	: filepath(filepath), comment_token("#"), is_writeable(is_writeable), eqtoken('=') {
	open_cfg_file(filepath, is_writeable);
	comment_token = "#";
}

cfgfile::cfgfile(const string &filepath, const string &comment_token = "#", char eqtoken = '=', bool is_writeable = false)
	: filepath(filepath), comment_token(comment_token), is_writeable(is_writeable), eqtoken(eqtoken) {
	open_cfg_file(filepath, is_writeable);
}

cfgfile::~cfgfile() {
	file.close();
}

void cfgfile::open_cfg_file(const string &filepath, bool writeable = false) {
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

string cfgfile::get_cfg_value(const string &cfg_identifier) const {
	if(!file.is_open()) {
		return "";
	}
	fstream tmpfile(filepath.c_str(), fstream::in); // second fstream to make const possible
	string buff, identstr, val;
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
			return val;
		}
	}
	return "";
}

bool cfgfile::set_cfg_value(const string &cfg_identifier, const string &cfg_value) {
	if(!is_writeable || !file.is_open()) {
		return false;
	}

	file.seekg(0);
	vector<string> cfgvec;
	string buff, newbuff, identstr, val;
	size_t eqloc;
	bool done = false;

	while(!file.eof() && file.good()) {
		getline(file, buff);
		cfgvec.push_back(buff);
	}

	for(vector<string>::iterator it = cfgvec.begin(); it != cfgvec.end(); ++it) {
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
	for(vector<string>::iterator it = cfgvec.begin(); it != cfgvec.end(); ++it) {
		file << *it << '\n';
	}

	reload_file();
	return true;
}

string cfgfile::get_comment_token() const {
	return comment_token;
}

void cfgfile::set_comment_token(const string &comment_token) {
	this->comment_token = comment_token;
}

void cfgfile::reload_file() {
	file.close();

	if(is_writeable) {
		file.open(this->filepath.c_str(), fstream::in | fstream::out | fstream::ate);
	} else {
		file.open(this->filepath.c_str(), fstream::in);
	}
}

void cfgfile::close_cfg_file() {
	file.close();
}

inline bool cfgfile::writeable() const {
	return is_writeable;
}

inline std::string cfgfile::get_filepath() const {
	return filepath;
}

bool cfgfile::list_config(std::string& resultstr) const {
	if(!file.is_open()) {
		return false;
	}
	fstream tmpfile(filepath.c_str(), fstream::in); // second fstream to make const possible
	string buff;
	resultstr = "";
	while(!tmpfile.eof() && tmpfile.good()) {
		getline(tmpfile, buff);
		trim(buff);
		if(buff.empty()) {
			continue;
		} else {
			if(buff.find(comment_token, 0) != string::npos) {
				buff = buff.substr(0, buff.find(comment_token, 0));
			}
			resultstr += buff;
			resultstr += '\n';
		}
	}
	return true;

}

void cfgfile::trim(string &str) const {
	while(isspace(str[0])) {
		str.erase(str.begin());
	}
	while(isspace(*(str.end() - 1))) {
		str.erase(str.end() -1);
	}
}