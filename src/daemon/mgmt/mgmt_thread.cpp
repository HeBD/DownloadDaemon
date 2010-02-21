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

#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <sstream>
#include <ctime>
#include <cstring>
#include <algorithm>

#include <sys/socket.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>

#include "mgmt_thread.h"
#include "../dl/download.h"
#include <netpptk/netpptk.h>
#include <cfgfile/cfgfile.h>
#include <crypt/md5.h>
#include "../tools/helperfunctions.h"
#include "../dl/download_container.h"
#include "../dl/download_thread.h"
#include "../global.h"
using namespace std;


/** the main thread for the management-interface over tcp
 */
void mgmt_thread_main() {
	tkSock main_sock(string_to_int(global_config.get_cfg_value("mgmt_max_connections")), 1024);
	int mgmt_port = string_to_int(global_config.get_cfg_value("mgmt_port"));
	if(mgmt_port == 0) {
		log_string("Unable to get management-socket-port from configuration file. defaulting to 56789", LOG_ERR);
		mgmt_port = 56789;
	}

	string bind_addr = global_config.get_cfg_value("bind_addr");

	if(!main_sock.bind(mgmt_port, bind_addr)) {
		log_string("bind failed for remote-management-socket. Maybe you specified a port wich is "
				   "already in use or you don't have read-permissions to the config-file.", LOG_ERR);
		exit(-1);
	}

	if(!main_sock.listen()) {
		log_string("Could not listen on management socket", LOG_ERR);
		exit(-1);
	}

	srand(time(NULL));

	main_sock.auto_accept(connection_handler);
	main_sock.auto_accept_join();

}

/** connection handle for management connections (callback function for tkSock)
 * @param sock the socket we get for communication with the other side
 */
void connection_handler(tkSock *sock) {
	std::string data;
	std::string passwd(global_config.get_cfg_value("mgmt_password"));
	if(*sock && passwd != "") {
		*sock << "102 AUTHENTICATION";
		*sock >> data;
		trim_string(data);
		bool auth_success = false;
		std::string auth_allowed = global_config.get_cfg_value("mgmt_accept_enc");
		if(auth_allowed != "plain" && auth_allowed != "encrypt") {
			auth_allowed = "both";
		}

		if(data == passwd && (auth_allowed == "plain" || auth_allowed == "both")) {
			auth_success = true;
		} else if(data == "ENCRYPT" && (auth_allowed == "both" || auth_allowed == "encrypt")) {
			std::string rnd;
			rnd.resize(128);
			for(int i = 0; i < 128; ++i) {
				rnd[i] = rand() % 255;
			}
			sock->send(rnd);
			std::string response_have;
			sock->recv(response_have);
			rnd += passwd;

			MD5_CTX md5;
			MD5_Init(&md5);
			unsigned char* enc_data = new unsigned char[rnd.length()];
			memset(enc_data, 0, 128);
			for(size_t i = 0; i < rnd.length(); ++i) {
				enc_data[i] = rnd[i];
			}

			MD5_Update(&md5, enc_data, rnd.length());
			unsigned char result[16];
			MD5_Final(result, &md5);
			std::string response_should((char*)result, 16);
			delete [] enc_data;

			if(response_should == response_have) {
				auth_success = true;
			}
		}

		if(!auth_success) {
			log_string("Authentication failed. " + sock->get_peer_name() + " entered a wrong password or used invalid encryption", LOG_WARNING);
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
		// dump after each command
		global_download_list.dump_to_file();
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
		} else if(data.find("PKG") == 0) {
			data = data.substr(3);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_pkg(data, sock);
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
		} else if(data.find("ROUTER") == 0) {
			data = data.substr(6);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_router(data, sock);
		} else if(data.find("PREMIUM") == 0) {
			data = data.substr(7);
			if(data.length() == 0 || !isspace(data[0])) {
				*sock << "101 PROTOCOL";
				continue;
			}
			trim_string(data);
			target_premium(data, sock);
		} else {
			*sock << "101 PROTOCOL";
		}
	}
}


void target_dl(std::string &data, tkSock *sock) {
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

void target_dl_list(std::string &data, tkSock *sock) {
	//log_string("Dumping download list to client", LOG_DEBUG);
	*sock << global_download_list.create_client_list();
}

void target_dl_add(std::string &data, tkSock *sock) {
	int package;
	std::string url;
	std::string comment;
	if(data.find_first_of("|\n\r") != string::npos) {
		*sock << "108 VARIABLE";
		return;
	} else {
		package = atoi(data.substr(0, data.find(' ')).c_str());
		data = data.substr(data.find(' '));
		trim_string(data);
		size_t n = data.find(' ');
		url = data.substr(0, n);
		if(n != string::npos)
			comment = data.substr(n);
		trim_string(data);
		trim_string(url);
		trim_string(comment);
	}
	if(validate_url(url)) {
		if(global_download_list.url_is_in_list(url)) {
			std::string refuse(global_config.get_cfg_value("refuse_existing_links"));
			if(refuse == "1" || refuse == "true") {
				*sock << "108 VARIABLE";
				return;
			}

		}
		download* dl = new download(url);
		dl->comment = comment;
		std::string logstr("Adding download: ");
		logstr += dl->serialize();
		logstr.erase(logstr.length() - 1);
		log_string(logstr, LOG_DEBUG);

		if(global_download_list.add_dl_to_pkg(dl, package) != LIST_SUCCESS) {
			log_string("Tried to add a download to a non-existant package", LOG_WARNING);
			*sock << "104 ID";
		} else {
			*sock << "100 SUCCESS";
		}
		return;
	}
	log_string("Could not add a download", LOG_WARNING);
	*sock << "103 URL";
}

void target_dl_del(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	global_download_list.set_status(id, DOWNLOAD_DELETED);
	*sock << "100 SUCCESS";
}

void target_dl_stop(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	global_download_list.set_need_stop(id, true);
	*sock << "100 SUCCESS";
}

void target_dl_up(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	global_download_list.move_dl(id, package_container::DIRECTION_UP);
	*sock << "100 SUCCESS";
}

void target_dl_down(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	global_download_list.move_dl(id, package_container::DIRECTION_DOWN);
	*sock << "100 SUCCESS";
}

void target_dl_activate(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	if(global_download_list.get_status(id) == DOWNLOAD_INACTIVE) {
		global_download_list.set_status(id, DOWNLOAD_PENDING);
		*sock << "100 SUCCESS";
	} else {
		*sock << "106 ACTIVATE";
	}
}

void target_dl_deactivate(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	if(global_download_list.get_status(id) == DOWNLOAD_INACTIVE) {
		*sock << "107 DEACTIVATE";
	} else {
		global_download_list.set_status(id, DOWNLOAD_INACTIVE);
		global_download_list.set_wait(id, 0);
		*sock << "100 SUCCESS";
	}
}

void target_pkg(std::string &data, tkSock *sock) {
	if(data.find("ADD") == 0) {
		data = data.substr(3);
		trim_string(data);
		target_pkg_add(data, sock);
	} else if(data.find("DEL") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_del(data, sock);
	} else if(data.find("UP") == 0) {
		data = data.substr(2);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_up(data, sock);
	} else if(data.find("DOWN") == 0) {
		data = data.substr(4);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_down(data, sock);
	} else if(data.find("EXISTS") == 0) {
		data = data.substr(6);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_pkg_exists(data, sock);
	} else {
		*sock << "101 PROTOCOL";
	}
}

void target_pkg_add(std::string &data, tkSock *sock) {
	if(data.find_first_of("|\n\r") != string::npos) {
		*sock << "-1";
		return;
	}
	*sock << int_to_string(global_download_list.add_package(data));
}

void target_pkg_del(std::string &data, tkSock *sock) {
	global_download_list.del_package(atoi(data.c_str()));
	*sock << "100 SUCCESS";
}

void target_pkg_up(std::string &data, tkSock *sock) {
	global_download_list.move_pkg(atoi(data.c_str()), package_container::DIRECTION_UP);
	*sock << "100 SUCCESS";
}

void target_pkg_down(std::string &data, tkSock *sock) {
	global_download_list.move_pkg(atoi(data.c_str()), package_container::DIRECTION_DOWN);
	*sock << "100 SUCCESS";
}

void target_pkg_exists(std::string &data, tkSock *sock) {
	*sock << int_to_string(global_download_list.pkg_exists(atoi(data.c_str())));
}

void target_var(std::string &data, tkSock *sock) {
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

void target_var_get(std::string &data, tkSock *sock) {
	if(data == "mgmt_password") {
		*sock << "";
	} else {
		*sock << global_config.get_cfg_value(data);
	}
}

void target_var_set(std::string &data, tkSock *sock) {
	if(data.find('=') == string::npos || data.find_first_of("\n\r") != string::npos) {
		*sock << "101 PROTOCOL";
		return;
	}
	if(data.find("mgmt_password") == 0) {
		if(data.find(';') == std::string::npos) {
			*sock << "101 PROTOCOL";
			return;
		}
		size_t pos1 = data.find('=') + 1;
		size_t pos2 = data.find(';', pos1);
		std::string old_pw;
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
				log_string("Unable to write configuration file", LOG_ERR);
				*sock << "110 PERMISSION";
			}

		} else {
			log_string("Unable to change management password", LOG_ERR);
			*sock << "102 AUTHENTICATION";
		}
	} else {
		std::string identifier(data.substr(0, data.find('=')));
		trim_string(identifier);
		std::string value(data.substr(data.find('=') + 1));
		trim_string(value);
		if(!proceed_variable(identifier, value)) {
			*sock << "111 VALUE";
			return;
		}

		if(variable_is_valid(identifier)) {
			if(global_config.set_cfg_value(identifier, value)) {
				*sock << "100 SUCCESS";
			} else {
				*sock << "110 PERMISSION";
			}
		} else {
			*sock << "108 VARIABLE";
		}
	}

}

void target_file(std::string &data, tkSock *sock) {
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
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	std::string fn = global_download_list.get_output_file(id);
	struct stat st;
	if(fn.empty() || stat(fn.c_str(), &st) != 0) {
		*sock << "109 FILE";
		return;
	}

	if(remove(fn.c_str()) != 0) {
		log_string(std::string("Failed to delete file ID: ") + data, LOG_WARNING);
		*sock << "110 PERMISSION";
	} else {
		log_string(std::string("Deleted file ID: ") + data, LOG_DEBUG);
		*sock << "100 SUCCESS";
		global_download_list.set_output_file(id, "");
	}
}

void target_file_getpath(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	std::string output_file = global_download_list.get_output_file(id);
	struct stat st;
	if(stat(output_file.c_str(), &st) != 0) {
		*sock << "";
		return;
	}
	*sock << output_file;
}

void target_file_getsize(std::string &data, tkSock *sock) {
	if(data.empty()) {
		*sock << "104 ID";
		return;
	}
	dlindex id;
	id.second = atoi(data.c_str());
	id.first = global_download_list.pkg_that_contains_download(id.second);
	if(id.first == LIST_ID) {
		*sock << "104 ID";
		return;
	}
	std::string output_file = global_download_list.get_output_file(id);

	struct stat st;
	if(stat(output_file.c_str(), &st) != 0) {
		log_string(std::string("Failed to get size of file ID: ") + data, LOG_WARNING);
		*sock << "-1";
		return;
	}
	*sock << int_to_string(st.st_size);
}

void target_router(std::string &data, tkSock *sock) {
	if(data.find("LIST") == 0) {
		data = data.substr(4);
		trim_string(data);
		target_router_list(data, sock);
	} else if(data.find("SETMODEL") == 0) {
		data = data.substr(8);
		trim_string(data);
		target_router_setmodel(data, sock);
	} else if(data.find("SET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_router_set(data, sock);
	} else if(data.find("GET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_router_get(data, sock);
	} else {
		*sock << "101 PROTOCOL";
	}
}

void target_router_list(std::string &data, tkSock *sock) {
	DIR *dp;
	struct dirent *ep;
	vector<std::string> content;
	std::string current;
	std::string path = program_root + "reconnect/";
	dp = opendir (path.c_str());
	if (dp == NULL) {
		log_string("Could not open reconnect script directory", LOG_ERR);
		*sock << "";
		return;
	}

	while ((ep = readdir (dp))) {
		if(ep->d_name[0] == '.') {
			continue;
		}
		current = ep->d_name;
		content.push_back(current);
	}

	(void) closedir (dp);
	sort(content.begin(), content.end(), CompareNoCase);
	std::string to_send;
	for(vector<std::string>::iterator it = content.begin(); it != content.end(); ++it) {
		to_send.append(*it);
		to_send.append("\n");
	}
	if(!to_send.empty()) {
		to_send.erase(to_send.end() - 1);
	}

	*sock << to_send;
}

void target_router_setmodel(std::string &data, tkSock *sock) {
	struct dirent *de = NULL;
	DIR *d = NULL;
	std::string dir = program_root + "reconnect/";
	d = opendir(dir.c_str());

	if(d == NULL) {
		log_string("Could not open reconnect plugin directory", LOG_ERR);
		*sock << "109 FILE";
		return;
	}

	bool plugin_found = true;
	if(!data.empty()) {
		plugin_found = false;
		while((de = readdir(d))) {
			if(data == de->d_name) {
				plugin_found = true;
			}
		}
	closedir(d);
	}

	if(!plugin_found) {
		log_string("Selected reconnect plugin not found", LOG_WARNING);
		*sock << "109 FILE";
		return;
	}

	if(global_router_config.set_cfg_value("router_model", data)) {
		log_string("Changed router model to " + data, LOG_DEBUG);
		*sock << "100 SUCCESS";
	} else {
		log_string("Unable to set router model", LOG_ERR);
		*sock << "110 PERMISSION";
	}
}

void target_router_set(std::string &data, tkSock *sock) {
	size_t eqpos = data.find('=');
	if(eqpos == std::string::npos) {
		*sock << "101 PROTOCOL";
		return;
	}
	std::string identifier = data.substr(0, eqpos);
	std::string variable = data.substr(eqpos + 1);
	trim_string(identifier);
	trim_string(variable);

	if(!router_variable_is_valid(identifier)) {
		log_string("Tried to change an invalid router variable", LOG_WARNING);
		*sock << "108 VARIABLE";
		return;
	}

	if(global_router_config.set_cfg_value(identifier, variable)) {
		log_string("Changed router variable", LOG_DEBUG);
		*sock << "100 SUCCESS";
	} else {
		log_string("Unable to set router variable!", LOG_ERR);
		*sock << "110 PERMISSION";
	}
}

void target_router_get(std::string &data, tkSock *sock) {
	if(data == "router_password") {
		*sock << "";
		return;
	} else {
		*sock << global_router_config.get_cfg_value(data);
	}
}


void target_premium(std::string &data, tkSock *sock) {
	if(data.find("LIST") == 0) {
		data = data.substr(4);
		trim_string(data);
		if(!data.empty()) {
			*sock << "";
			return;
		}
		target_premium_list(data, sock);
	} else if(data.find("GET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_premium_get(data, sock);
	} else if(data.find("SET") == 0) {
		data = data.substr(3);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
		trim_string(data);
		target_premium_set(data, sock);
	}
}

void target_premium_list(std::string &data, tkSock *sock) {
	DIR *dp;
	struct dirent *ep;
	vector<std::string> content;
	std::string current;
	std::string path = program_root + "/plugins/";
	correct_path(path);
	path += '/';
	dp = opendir (path.c_str());
	if (dp == NULL) {
		log_string("Could not open Plugin directory", LOG_ERR);
		*sock << "";
		return;
	}
	plugin_input inp;
	plugin_output outp;

	while ((ep = readdir (dp))) {
		if(ep->d_name[0] == '.') {
			continue;
		}
		current = ep->d_name;
		if(current.find("lib") == 0 && current.find(".so") != string::npos) {
			// open each plugin and check if it supports premium stuff
			void* handle = dlopen((path + current).c_str(), RTLD_LAZY);
			if (!handle) {
				log_string(std::string("Unable to open library file: ") + dlerror() + '/' + current, LOG_ERR);
				continue;
			}
			dlerror();	// Clear any existing error
			void (*plugin_getinfo)(plugin_input&, plugin_output&);
			plugin_getinfo = (void (*)(plugin_input&, plugin_output&))dlsym(handle, "plugin_getinfo");
			char *l_error;
			if ((l_error = dlerror()) != NULL)  {
				log_string(std::string("Unable to get plugin information: ") + l_error, LOG_ERR);
				dlclose(handle);
				continue;
			}
			outp.offers_premium = false;
			plugin_getinfo(inp, outp);
			dlclose(handle);
			if(outp.offers_premium) {
				current = current.substr(3, current.length() - 6);
				content.push_back(current);
			}

		}
	}
	(void) closedir (dp);
	std::string to_send;
	for(vector<std::string>::iterator it = content.begin(); it != content.end(); ++it) {
		to_send.append(*it);
		to_send.append("\n");
	}
	if(!to_send.empty()) {
		to_send.erase(to_send.end() - 1);
	}
	*sock << to_send;
}

void target_premium_get(std::string &data, tkSock *sock) {
	// that's really all. if an error occurs, get_cfg_value's return value is an empty string - perfect
	*sock << global_premium_config.get_cfg_value(data + "_user");
}

void target_premium_set(std::string &data, tkSock *sock) {
	std::string user;
	std::string password;
	std::string host;
	if(data.find_first_of(" \n\t\r") == string::npos) {
		*sock << "101 PROTOCOL";
		return;
	}
	host = data.substr(0, data.find_first_of(" \n\t\r"));
	data = data.substr(host.length());
	trim_string(data);
	if(data.find(';') == string::npos) {
		*sock << "101 PROTOCOL";
		return;
	}
	user = data.substr(0, data.find(';'));
	data = data.substr(data.find(';'));
	if(data.length() > 1) {
		password = data.substr(data.find(';') + 1);
	}
	if(global_premium_config.set_cfg_value(host + "_user", user) && global_premium_config.set_cfg_value(host + "_password", password)) {
		*sock << "100 SUCCESS";
	} else {
		*sock << "110 PERMISSION";
	}

}
