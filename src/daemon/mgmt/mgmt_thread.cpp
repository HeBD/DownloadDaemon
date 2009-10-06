/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include<iostream>
#include<vector>
#include<cassert>
#include<string>
#include<sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>

#include "mgmt_thread.h"
#include "../dl/download.h"
#include "../../lib/netpptk/netpptk.h"
#include "../../lib/cfgfile/cfgfile.h"
#include "../tools/helperfunctions.h"
#include "../dl/download_container.h"
#include "../dl/download_thread.h"

using namespace std;

extern cfgfile global_config;
extern cfgfile global_router_config;
extern cfgfile global_premium_config;
extern download_container global_download_list;
extern std::string program_root;



/** the main thread for the management-interface over tcp
 */
void mgmt_thread_main() {
	tkSock main_sock(string_to_int(global_config.get_cfg_value("mgmt_max_connections")), 1024);
	if(global_config.get_cfg_value("mgmt_port") == "") {
		log_string("Unable to get management-socket-port from configuration file.", LOG_SEVERE);
		exit(-1);
	}
	if(!main_sock.bind(string_to_int(global_config.get_cfg_value("mgmt_port")))) {
		log_string("bind failed for remote-management-socket. Maybe you specified a port wich is "
				   "already in use or you don't have read-permissions to the config-file.", LOG_SEVERE);
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
 * @param sock the socket we get for communication with the other side
 */
void connection_handler(tkSock *sock) {
	std::string data;
	std::string passwd(global_config.get_cfg_value("mgmt_password"));
	if(*sock && passwd != "") {
		*sock << "102 AUTHENTICATION";
		*sock >> data;
		if(data != passwd) {
			log_string("Authentication failed. " + sock->get_peer_name() + " entered a wrong password", LOG_WARNING);
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
	std::string url;
	std::string comment;
	if(data.find(' ') == std::string::npos) {
		url = data;
	} else {
		url = data.substr(0, data.find(' '));
		comment = data.substr(data.find(' '), std::string::npos);
		replace_all(comment, "|", "\\|");
		trim_string(url);
		trim_string(comment);
	}
	if(validate_url(url)) {
		download dl(url, global_download_list.get_next_id());
		dl.comment = comment;
		std::string logstr("Adding download: ");
		logstr += dl.serialize();
		logstr.erase(logstr.length() - 1);
		log_string(logstr, LOG_DEBUG);
		if(global_download_list.add_download(dl) != LIST_SUCCESS) {
			*sock << "110 PERMISSION";
		} else {
			*sock << "100 SUCCESS";
		}
		return;
	}
	log_string("Could not add a download", LOG_WARNING);
	*sock << "103 URL";
}

void target_dl_del(std::string &data, tkSock *sock) {
	switch(global_download_list.set_int_property(string_to_int(data), DL_STATUS, DOWNLOAD_DELETED)) {
		case LIST_PERMISSION:
			log_string("Failed to remove download ID: " + data, LOG_SEVERE);
			*sock << "110 PERMISSION";
		break;
		case LIST_SUCCESS:
			log_string("Removing download ID: " + data, LOG_DEBUG);
			*sock << "100 SUCCESS";
		break;
		default:
			log_string("Failed to remove download ID: " + data, LOG_SEVERE);
			*sock << "104 ID";
		break;
	}
}

void target_dl_stop(std::string &data, tkSock *sock) {
	switch(global_download_list.stop_download(atoi(data.c_str()))) {
		case LIST_PERMISSION:
			log_string(std::string("Failed to stop download ID: ") + data, LOG_SEVERE);
			*sock << "110 PERMISSION";
		break;
		case LIST_SUCCESS:
			log_string(std::string("Stopped download ID: ") + data, LOG_DEBUG);
			*sock << "100 SUCCESS";
		break;
		default:
			log_string("Failed to remove download ID: " + data, LOG_SEVERE);
			*sock << "104 ID";
		break;
	}
}

void target_dl_up(std::string &data, tkSock *sock) {
	switch (global_download_list.move_up(atoi(data.c_str()))) {
		case LIST_SUCCESS:
			log_string(std::string("Moved download ID: ") + data + " upwards", LOG_DEBUG);
			*sock << "100 SUCCESS"; break;
		case LIST_ID:
			log_string(std::string("Failed to move download ID: ") + data + " upwards", LOG_SEVERE);
			*sock << "104 ID"; break;
		case LIST_PERMISSION:
			log_string(std::string("Failed to move download ID: ") + data + " upwards", LOG_SEVERE);
			*sock << "110 PERMISSION"; break;
	}
}

void target_dl_down(std::string &data, tkSock *sock) {
	switch (global_download_list.move_down(atoi(data.c_str()))) {
		case LIST_SUCCESS:
			log_string(std::string("Moved download ID: ") + data + " downwards", LOG_DEBUG);
			*sock << "100 SUCCESS";
		break;
		case LIST_ID:
			log_string(std::string("Failed to move download ID: ") + data + " downwards", LOG_SEVERE);
			*sock << "104 ID";
		break;
		case LIST_PERMISSION:
			log_string(std::string("Failed to move download ID: ") + data + " downwards", LOG_SEVERE);
			*sock << "110 PERMISSION";
		break;
	}
}

void target_dl_activate(std::string &data, tkSock *sock) {
	switch(global_download_list.activate(atoi(data.c_str()))) {
		case LIST_ID:
			*sock << "104 ID";
			log_string(std::string("Failed to activate download ID: ") + data, LOG_SEVERE);
		break;
		case LIST_PERMISSION:
			log_string(std::string("Failed to activate download ID: ") + data, LOG_SEVERE);
			*sock << "110 PERMISSION";
		break;
		case LIST_SUCCESS:
			log_string(std::string("Activated download ID: ") + data, LOG_DEBUG);
			*sock << "100 SUCCESS";
		break;
		default:
			*sock << "106 ACTIVATE";
			log_string(std::string("Failed to activate download ID: ") + data, LOG_WARNING);
		break;

	}
}

void target_dl_deactivate(std::string &data, tkSock *sock) {
	switch(global_download_list.deactivate(atoi(data.c_str()))) {
		case LIST_ID:
			*sock << "104 ID";
			log_string(std::string("Failed to deactivate download ID: ") + data, LOG_SEVERE);
		break;
		case LIST_PERMISSION:
			log_string(std::string("Failed to deactivate download ID: ") + data, LOG_SEVERE);
			*sock << "110 PERMISSION";
		break;
		case LIST_SUCCESS:
			log_string(std::string("Deactivated download ID: ") + data, LOG_DEBUG);
			*sock << "100 SUCCESS";
		break;
		default:
			*sock << "107 DEACTIVATE";
			log_string(std::string("Failed to deactivate download ID: ") + data, LOG_WARNING);
		break;

	}
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
	if(!data.find('=')) {
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
				log_string("Unable to write configuration file", LOG_SEVERE);
				*sock << "110 PERMISSION";
			}

		} else {
			log_string("Unable to change management password", LOG_SEVERE);
			*sock << "102 AUTHENTICATION";
		}
	} else {
		std::string identifier(data.substr(0, data.find('=')));
		trim_string(identifier);
		std::string value(data.substr(data.find('=') + 1));
		trim_string(value);
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
	std::string output_file;
	try {
		output_file = global_download_list.get_string_property(atoi(data.c_str()), DL_OUTPUT_FILE);
	} catch(download_exception &e) {
		log_string(std::string("Failed to delete file ID: ") + data, LOG_WARNING);
		*sock << "104 ID";
		return;
	}

	if(output_file == "" || global_download_list.get_int_property(atoi(data.c_str()), DL_STATUS) == DOWNLOAD_RUNNING) {
		log_string(std::string("Failed to delete file ID: ") + data, LOG_WARNING);
		*sock << "109 FILE";
		return;
	}

	if(remove(output_file.c_str()) == 0) {
		log_string(std::string("Deleted file ID: ") + data, LOG_DEBUG);
		*sock << "100 SUCCESS";
		global_download_list.set_string_property(atoi(data.c_str()), DL_OUTPUT_FILE, "");
	} else {
		*sock << "109 FILE";
	}
}

void target_file_getpath(std::string &data, tkSock *sock) {
	std::string output_file;
	try {
		output_file = global_download_list.get_string_property(atoi(data.c_str()), DL_OUTPUT_FILE);
	} catch(download_exception &e) {
		log_string(std::string("Failed to get path of file ID: ") + data, LOG_WARNING);
		*sock << "";
		return;
	}

	struct stat st;
	if(stat(output_file.c_str(), &st) != 0) {
		*sock << "";
		return;
	}
	*sock << output_file;
}

void target_file_getsize(std::string &data, tkSock *sock) {
	std::string output_file;
	try {
		output_file = global_download_list.get_string_property(atoi(data.c_str()), DL_OUTPUT_FILE);
	} catch(download_exception &e) {
		log_string(std::string("Failed to get size of file ID: ") + data, LOG_WARNING);
		*sock << "-1";
		return;
	}

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
		log_string("Could not open reconnect script directory", LOG_SEVERE);
		*sock << "";
		return;
	}

	while ((ep = readdir (dp))) {
		if(ep->d_name[0] == '.') {
			continue;
		}
		current = ep->d_name;
		if(current.find("lib") == 0) {
			current = current.substr(3);
			if(current.find(".so") != std::string::npos) {
				current = current.substr(0, current.find(".so"));
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

void target_router_setmodel(std::string &data, tkSock *sock) {
	struct dirent *de = NULL;
	DIR *d = NULL;
	std::string dir = program_root + "reconnect/";
	d = opendir(dir.c_str());

	if(d == NULL) {
		log_string("Could not open reconnect plugin directory", LOG_SEVERE);
		*sock << "109 FILE";
		return;
	}

	bool plugin_found = true;
	if(!data.empty()) {
		std::string searched_plugin(data);
		searched_plugin.append(".so");
		searched_plugin.insert(0, "lib");
		plugin_found = false;
		while((de = readdir(d))) {
			if(searched_plugin == de->d_name) {
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
		log_string("Unable to set router model", LOG_SEVERE);
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
		log_string("Unable to set router variable!", LOG_SEVERE);
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
	std::string path = global_config.get_cfg_value("plugin_dir");
	correct_path(path);
	dp = opendir (path.c_str());
	if (dp == NULL) {
		log_string("Could not open Plugin directory", LOG_SEVERE);
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
				log_string(std::string("Unable to open library file: ") + dlerror() + '/' + current, LOG_SEVERE);
				continue;
			}
			dlerror();	// Clear any existing error
			void (*plugin_getinfo)(plugin_input&, plugin_output&);
			plugin_getinfo = (void (*)(plugin_input&, plugin_output&))dlsym(handle, "plugin_getinfo");
			char *l_error;
			if ((l_error = dlerror()) != NULL)  {
				log_string(std::string("Unable to get plugin information: ") + l_error, LOG_SEVERE);
				return;
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
	if(global_premium_config.set_cfg_value(host + "_user", user) && global_premium_config.set_cfg_value(host + "_passowrd", password)) {
		*sock << "100 SUCCESS";
	} else {
		*sock << "110 PERMISSION";
	}

}
