#include<iostream>
#include<vector>
#include<cassert>
#include<string>

#include "mgmt_thread.h"
#include "../dl/download.h"
#include "../../lib/netpptk/netpptk.h"
#include "../../lib/cfgfile/cfgfile.h"
#include "../tools/helperfunctions.h"
#include "../dl/download_container.h"

using namespace std;

extern cfgfile global_config;
extern download_container global_download_list;
extern std::string program_root;



/** the main thread for the management-interface over tcp
 * @TODO REPLACE ERRORS WITH CORRECT ERROR CODES / IMPLEMENT LIB
 */
void mgmt_thread_main() {
	tkSock main_sock(string_to_int(global_config.get_cfg_value("mgmt_max_connections")), 1024);
	if(!main_sock.bind(string_to_int(global_config.get_cfg_value("mgmt_port")))) {
		log_string("Could not bind management socket", LOG_SEVERE);
		exit(-1);
	}
	if(!main_sock.listen()) {
		log_string("Could not listen on management socket", LOG_SEVERE);
	}

	main_sock.auto_accept(connection_handler);
	main_sock.auto_accept_join();

}

/** connection handle for management connections (callback function for tkSock)
 * @param the socket we get for communication with the other side
 */
void connection_handler(tkSock *sock) {
	std::string data;
	std::string passwd(global_config.get_cfg_value("mgmt_password"));
	bool auth_success = false;;
	if(*sock && passwd != "") {
		*sock << "1 AUTH REQUIRED";
		*sock >> data;
		if(data != passwd) {
			auth_success = false;
			log_string("Authentication failed", LOG_WARNING);
			log_string(string("Received instead of real password: ") + data, LOG_DEBUG);
			if(*sock) *sock << "-1 ERROR";
			return;
		} else {
			log_string("User Authenticated", LOG_DEBUG);
			auth_success = true;
			if(*sock) *sock << "0 SUCCESS";
		}
	} else if(*sock) {
		*sock << "0 NO AUTH";
		auth_success =true;
	}
	while(*sock) {
		if(auth_success == false) {
			break;
		}
		if(sock->recv(data) == 0) {
			return;
		}
		trim_string(data);

		if(data.length() < 8 || data.find("DDP") != 0 || !isspace(data[3])) {
			if(*sock) *sock << "-1 ERROR";
			break;
		}
		data = data.substr(4);
		trim_string(data);
		bool do_answer = false;
		bool the_answer;

		if(data.find("ADD") == 0) {
			do_answer = true;
			the_answer = add(data.substr(4));
		} else if(data.find("DEL") == 0) {
			do_answer = true;
			the_answer = del(data.substr(4));
		} else if(data.find("GET") == 0) {
			do_answer = false;
			get(data.substr(4), *sock);
		} else if(data.find("SET") == 0) {
			do_answer = true;
			the_answer = set(data.substr(4));
		} else {
			do_answer = true;
			the_answer = false;
		}
		if(do_answer) {
			if(the_answer) {
				if(*sock) *sock << "0 SUCCESS";
			} else {
				if(*sock) *sock << "-1 ERROR";
				break;
			}
		}
	}
}

/** adds a download to the queue */
bool add(std::string data) {
	trim_string(data);
	string url;
	string comment;
	if(data.find(' ') == string::npos) {
		url = data;
	} else {
		url = data.substr(0, data.find(' '));
		comment = data.substr(data.find(' '), string::npos);
		replace_all(comment, "|", "\\|");
		replace_all(url, "|", "\\|");
		trim_string(url);
		trim_string(comment);
	}
	if(validate_url(url)) {

		download dl(url, get_next_id());
		dl.comment = comment;
		string logstr("Adding download: ");
		logstr += dl.serialize();
		log_string(logstr, LOG_DEBUG);

		return global_download_list.push_back(dl);
	}
	log_string("Could not add a download", LOG_WARNING);
	return false;
}

/** deletes a download from the queue */
bool del(std::string data) {
	trim_string(data);

	string logstr("Removing download with id: ");
	logstr += data;
	log_string(logstr, LOG_DEBUG);
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	curl_easy_setopt(it->handle, CURLOPT_TIMEOUT, 1);
	it->set_status(DOWNLOAD_DELETED);
	return true;
}

/** Get LIST of downloads or a specific VAR <...> (or VAR LIST) */
bool get(std::string data, tkSock& sock) {
	trim_string(data);
	if(data == "LIST") {
		stringstream ss;
		for(download_container::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
			if(it->get_status() == DOWNLOAD_DELETED) {
				continue;
			}
			ss << it->id << '|' << it->add_date << '|';
			string comment = it->comment;
			replace_all(comment, "|", "\\|");
			ss << comment << '|' << it->url << '|' << it->get_status_str() << '|' << it->downloaded_bytes << '|' << it->size
			   << '|' << it->wait_seconds << '|' << it->get_error_str() << '\n';
		}
		//log_string("Dumping download list to client", LOG_DEBUG);
		sock << ss.str();
		return true;
	}
	if(data.find("VAR") == 0) {
		data = data.substr(3);
		trim_string(data);
		if(data == "LIST") {
			std::string confstr;
			if(!global_config.list_config(confstr)) {
				sock << "-1 ERROR";
				return false;
			}
			sock << confstr;
			return true;
		} else {
			sock << global_config.get_cfg_value(data);
			return true;
		}
	}
	return false;
}

/** Sets a config variable or download property */
bool set(std::string data) {
	trim_string(data);
	if(data.find("VAR") == 0) {
		data = data.substr(3);
		trim_string(data);
		string identifier = data.substr(0, data.find('='));
		trim_string(identifier);
		string value = data.substr(data.find('=') + 1);
		trim_string(value);
		if(!global_config.set_cfg_value(identifier, value)) {
			log_string("SET VAR failed", LOG_SEVERE);
			return false;
		}
		string logstr("Set Var: ");
		logstr += identifier + " to " + value;
		log_string(logstr, LOG_DEBUG);
		return true;
	}
	if(data.find("DL") == 0) {
		data = data.substr(2);
		trim_string(data);
		if(data.find(' ') == string::npos) return false;
		int id = atoi((data.substr(0, data.find(' '))).c_str());
		download_container::iterator it = global_download_list.get_download_by_id(id);

		data = data.substr(data.find(' '));
		trim_string(data);

		if(data.find("ACTIVE") == 0) {
			download_status old_status = it->get_status();
			data = data.substr(6);
			trim_string(data);
			if(data == "0") {
				if(it->get_status() == DOWNLOAD_RUNNING) {
					curl_easy_setopt(it->handle, CURLOPT_TIMEOUT, 1);
				}
				it->set_status(DOWNLOAD_INACTIVE);
				it->wait_seconds = 0;
			}
			else if(data == "1") {
				if(it->get_status() == DOWNLOAD_INACTIVE) {
					it->set_status(DOWNLOAD_PENDING);
				}
			} else {
				return false;
			}
			if(!global_download_list.dump_to_file()) {
				it->set_status(old_status);
				return false;
			}
			return true;

		} else if(data.find("COMMENT") == 0) {
			string old_comment = it->comment;
			data = data.substr(7);
			trim_string(data);
			it->comment = data;
			if(!global_download_list.dump_to_file()) {
				it->comment = old_comment;
				return false;
			}
			return true;

		} else if(data.find("URL") == 0) {
			string old_url = it->url;
			data = data.substr(3);
			trim_string(data);
			it->url = data;
			if(!global_download_list.dump_to_file()) {
				it->url = old_url;
				return false;
			}
			return true;
		}
		return false;
	}

	return false;
}

unsigned int get_next_id() {
	int max_id = 0;

	for(vector<download>::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
		if(it->id > max_id) {
			max_id = it->id;
		}
	}
	return ++max_id;
}


