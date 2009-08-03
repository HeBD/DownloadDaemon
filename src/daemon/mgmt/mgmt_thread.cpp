#include<iostream>
#include<vector>
#include<cassert>
#include<string>
#include<sstream>

#include <sys/types.h>
#include <sys/stat.h>

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
		exit(-1);
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
	if(*sock && passwd != "") {
		*sock << "102 AUTHENTICATION";
		*sock >> data;
		if(data != passwd) {
			log_string("Authentication failed", LOG_WARNING);
			log_string(string("Received instead of real password: ") + data, LOG_DEBUG);
			if(*sock) *sock << "102 AUTHENTICATION";
			return;
		} else {
			//log_string("User Authenticated", LOG_DEBUG);
			if(*sock) *sock << "100 SUCCESS";
		}
	} else if(*sock) {
		*sock << "100 SUCCESS";
	}
	while(*sock) {
		if(sock->recv(data) == 0) {
			*sock << "101 PROTOCOL";
			break;
		}
		trim_string(data);

		if(data.length() < 8 || data.find("DDP") != 0 || !isspace(data[3])) {
			if(*sock) *sock << "101 PROTOCOL";
			continue;
		}
		data = data.substr(4);
		trim_string(data);

		if(data.find("DL") == 0) {
			data = data.substr(2);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_dl(data, sock);
		} else if(data.find("VAR") == 0) {
			data = data.substr(3);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_var(data, sock);
		} else if(data.find("FILE") == 0) {
			data = data.substr(4);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_file(data, sock);
		} else {
			*sock << "101 PROTOCOL";
		}
	}
}


void target_dl(string &data, tkSock *sock) {
	if(data.find("LIST") == 0) {
		data = data.substr(4);
		trim_string(data);
		target_dl_list(data, sock);

	} else if(data.find("ADD") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_dl_add(data, sock);

	} else if(data.find("DEL") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		target_dl_del(data, sock);

	} else if(data.find("STOP") == 0) {
		data = data.substr(4);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_dl_stop(data, sock);

	} else if(data.find("UP") == 0) {
		data = data.substr(2);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_dl_up(data, sock);

	} else if(data.find("DOWN") == 0) {
		data = data.substr(4);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_dl_down(data, sock);

	} else if(data.find("ACTIVATE") == 0) {
		data = data.substr(8);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_dl_activate(data, sock);

	} else if(data.find("DEACTIVATE") == 0) {
		data = data.substr(10);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_dl_deactivate(data, sock);
	} else {
		*sock << "101 PROTOCOL";
	}

}

void target_dl_list(string &data, tkSock *sock) {
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
	*sock << ss.str();
}

void target_dl_add(string &data, tkSock *sock) {
	string url;
	string comment;
	if(data.find(' ') == string::npos) {
		url = data;
	} else {
		url = data.substr(0, data.find(' '));
		comment = data.substr(data.find(' '), string::npos);
		replace_all(comment, "|", "\\|");
		trim_string(url);
		trim_string(comment);
	}
	if(validate_url(url)) {
		download dl(url, global_download_list.get_next_id());
		dl.comment = comment;
		string logstr("Adding download: ");
		logstr += dl.serialize();
		logstr.erase(logstr.length() - 1);
		log_string(logstr, LOG_DEBUG);
		if(!global_download_list.push_back(dl)) {
			*sock << "110 PERMISSION";
		} else {
			*sock << "100 SUCCESS";
		}
		return;
	}
	log_string("Could not add a download", LOG_WARNING);
	*sock << "103 URL";
}

void target_dl_del(string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		*sock << "104 ID";
		log_string(string("Failed to remove download ID: ") + data, LOG_SEVERE);
	} else {
		curl_easy_setopt(it->handle, CURLOPT_TIMEOUT, 1);
		it->set_status(DOWNLOAD_DELETED);
		global_download_list.dump_to_file();
		log_string(string("Removing download ID: ") + data, LOG_DEBUG);
		*sock << "100 SUCCESS";
	}
}

void target_dl_stop(string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		*sock << "104 ID";
		log_string(string("Failed to stop download ID: ") + data, LOG_SEVERE);
	} else {
		curl_easy_setopt(it->handle, CURLOPT_TIMEOUT, 1);
		if(it->get_status() != DOWNLOAD_INACTIVE) it->set_status(DOWNLOAD_PENDING);
		global_download_list.dump_to_file();
		log_string(string("Stopped download ID: ") + data, LOG_DEBUG);
		*sock << "100 SUCCESS";
	}
}

void target_dl_up(string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	switch (global_download_list.move_up(atoi(data.c_str()))) {
		case 0:
			log_string(string("Moved download ID: ") + data + " upwards", LOG_DEBUG);
			*sock << "100 SUCCESS"; break;
		case -1:
			log_string(string("Failed to move download ID: ") + data + " upwards", LOG_SEVERE);
			*sock << "104 ID"; break;
		case -2:
			log_string(string("Failed to move download ID: ") + data + " upwards", LOG_SEVERE);
			*sock << "110 PERMISSION"; break;
	}
}

void target_dl_down(string &data, tkSock *sock) {
	// implemented with move_up. using ++ to get the next download and move it up, in order to move the
	// current download down.
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end() || ++it == global_download_list.end()) {
		log_string(string("Failed to move download ID: ") + data + " downwards.", LOG_SEVERE);
		*sock << "104 ID";
		return;
	}
	switch(global_download_list.move_up(atoi(data.c_str()))) {
		case 0:
			log_string(string("Moved download ID: ") + data + " downwards", LOG_DEBUG);
			*sock << "100 SUCCESS"; break;
		case -1:
			log_string(string("Failed to move download ID: ") + data + " downwards", LOG_SEVERE);
			*sock << "104 ID"; break;
		case -2:
			log_string(string("Failed to move download ID: ") + data + " downwards", LOG_SEVERE);
			*sock << "110 PERMISSION"; break;
	}
}

void target_dl_activate(string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		*sock << "104 ID";
		log_string(string("Failed to activate download ID: ") + data, LOG_SEVERE);
	} else if(it->get_status() != DOWNLOAD_INACTIVE) {
		*sock << "106 ACTIVATE";
		log_string(string("Failed to activate download ID: ") + data, LOG_WARNING);
	} else {
		it->set_status(DOWNLOAD_PENDING);
		if(!global_download_list.dump_to_file()) {
			it->set_status(DOWNLOAD_INACTIVE);
			log_string(string("Failed to activate download ID: ") + data, LOG_SEVERE);
			*sock << "110 PERMISSION";
		} else {
			log_string(string("Activated download ID: ") + data, LOG_DEBUG);
			*sock << "100 SUCCESS";
		}
	}
}

void target_dl_deactivate(string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		*sock << "104 ID";
		log_string(string("Failed to deactivate download ID: ") + data, LOG_SEVERE);
	} else {
		download_status old_status = it->get_status();
		if(it->get_status() == DOWNLOAD_RUNNING) {
			curl_easy_setopt(it->handle, CURLOPT_TIMEOUT, 1);
		}
		it->set_status(DOWNLOAD_INACTIVE);
		it->wait_seconds = 0;
		it->size = 0;
		it->downloaded_bytes = 0;
		if(!global_download_list.dump_to_file()) {
			it->set_status(old_status);
			log_string(string("Failed to deactivate download ID: ") + data, LOG_SEVERE);
			*sock << "110 PERMISSION";
		} else {
			log_string(string("Deactivated download ID: ") + data, LOG_DEBUG);
			*sock << "100 SUCCESS";
		}
	}
}

void target_var(string &data, tkSock *sock) {
	if(data.find("GET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_var_get(data, sock);
	} else if(data.find("SET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_var_set(data, sock);
	}
	else {
		*sock << "101 PROTOCOL";
	}
}

void target_var_get(string &data, tkSock *sock) {
	if(data == "mgmt_password") {
		*sock << "";
	} else {
		*sock << global_config.get_cfg_value(data);
	}
}

void target_var_set(string &data, tkSock *sock) {
	if(!data.find('=')) {
		*sock << "101 PROTOCOL";
		return;
	}
	if(data.find("mgmt_password") == 0) {
		if(data.find(';') == string::npos) {
			*sock << "101 PROTOCOL";
			return;
		}
		size_t pos1 = data.find('=') + 1;
		size_t pos2 = data.find(';', pos1);
		string old_pw;
		if(pos1 == pos2) {
			old_pw = "";
		} else {
			old_pw = data.substr(pos1, pos2 - pos1);
		}
		trim_string(old_pw);
		if(old_pw == global_config.get_cfg_value("mgmt_password")) {
			if(global_config.set_cfg_value("mgmt_password", data.substr(data.find(';') + 1))) {
				log_string("Changed management password", LOG_WARNING);
				*sock << "100 SUCCESS";
			} else {
				log_string("Unable to write configuration file", LOG_SEVERE);
				*sock << "110 PERMISSION";
			}

		} else {
			log_string("Unable to change management password", LOG_SEVERE);
			*sock << "102 AUTHENTICATION";
		}
	} else {
		if(variable_is_valid(data.substr(0, data.find('=') - 1))) {
			if(global_config.set_cfg_value(data.substr(0, data.find('=')), data.substr(data.find('=') + 1))) {
				*sock << "100 SUCCESS";
			} else {
				*sock << "110 PERMISSION";
			}
		} else {
			*sock << "108 VARIABLE";
		}
	}

}

void target_file(string &data, tkSock *sock) {
	if(data.find("DEL") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_file_del(data, sock);
	} else if(data.find("GETPATH") == 0) {
		data = data.substr(7);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_file_getpath(data, sock);
	} else if(data.find("GETSIZE") == 0) {
		data = data.substr(7);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_file_getsize(data, sock);
	} else {
		*sock << "101 PROTOCOL";
	}



}

void target_file_del(std::string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		log_string(string("Failed to delete file ID: ") + data, LOG_WARNING);
		*sock << "104 ID";
		return;
	}
	if(it->output_file == "" || it->get_status() == DOWNLOAD_RUNNING) {
		log_string(string("Failed to delete file ID: ") + data, LOG_WARNING);
		*sock << "109 FILE";
		return;
	}
	if(remove(it->output_file.c_str()) == 0) {
		log_string(string("Deleted file ID: ") + data, LOG_DEBUG);
		*sock << "100 SUCCESS";
		it->output_file = "";
	} else {
		*sock << "109 FILE";
	}
}

void target_file_getpath(std::string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		log_string(string("Failed to get path of file ID: ") + data, LOG_WARNING);
		*sock << "";
		return;
	}
	struct stat st;
	if(stat(it->output_file.c_str(), &st) != 0) {
		log_string(string("Failed to get path of file ID: ") + data, LOG_WARNING);
		*sock << "";
		return;
	}
	*sock << it->output_file;
}

void target_file_getsize(std::string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		log_string(string("Failed to get size of file ID: ") + data, LOG_WARNING);
		*sock << "-1";
		return;
	}
	struct stat st;
	if(stat(it->output_file.c_str(), &st) != 0) {
		log_string(string("Failed to get size of file ID: ") + data, LOG_WARNING);
		*sock << "-1";
		return;
	}
	*sock << int_to_string(st.st_size);

}







