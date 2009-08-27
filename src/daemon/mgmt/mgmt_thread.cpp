#include<iostream>
#include<vector>
#include<cassert>
#include<string>
#include<sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "mgmt_thread.h"
#include "../dl/download.h"
#include "../../lib/netpptk/netpptk.h"
#include "../../lib/cfgfile/cfgfile.h"
#include "../tools/helperfunctions.h"
#include "../dl/download_container.h"

using namespace std;

extern cfgfile global_config;
extern cfgfile global_router_config;
extern download_container global_download_list;
extern mt_string program_root;



/** the main thread for the management-interface over tcp
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
 * @param sock the socket we get for communication with the other side
 */
void connection_handler(tkSock *sock) {
	mt_string data;
	mt_string passwd(global_config.get_cfg_value("mgmt_password"));
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
        } else {
			*sock << "101 PROTOCOL";
		}
	}
}


void target_dl(mt_string &data, tkSock *sock) {
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

void target_dl_list(mt_string &data, tkSock *sock) {
	stringstream ss;
	for(download_container::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
		if(it->get_status() == DOWNLOAD_DELETED) {
			continue;
		}
		ss << it->get_id() << '|' << it->get_add_date() << '|';
		mt_string comment = it->get_comment();
		replace_all(comment, "|", "\\|");
		ss << comment << '|' << it->get_url() << '|' << it->get_status_str() << '|' << it->get_downloaded_bytes() << '|' << it->get_size()
		   << '|' << it->get_wait_seconds() << '|' << it->get_error_str() << '\n';
	}
	//log_string("Dumping download list to client", LOG_DEBUG);
	*sock << ss.str();
}

void target_dl_add(mt_string &data, tkSock *sock) {
	mt_string url;
	mt_string comment;
	if(data.find(' ') == mt_string::npos) {
		url = data;
	} else {
		url = data.substr(0, data.find(' '));
		comment = data.substr(data.find(' '), mt_string::npos);
		replace_all(comment, "|", "\\|");
		trim_string(url);
		trim_string(comment);
	}
	if(validate_url(url)) {
		download dl(url, global_download_list.get_next_id());
		dl.set_comment(comment);
		mt_string logstr("Adding download: ");
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

void target_dl_del(mt_string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		*sock << "104 ID";
		log_string(mt_string("Failed to remove download ID: ") + data, LOG_SEVERE);
	} else {
		curl_easy_setopt(it->get_handle(), CURLOPT_TIMEOUT, 1);
		it->set_status(DOWNLOAD_DELETED);
		global_download_list.dump_to_file();
		log_string(mt_string("Removing download ID: ") + data, LOG_DEBUG);
		*sock << "100 SUCCESS";
	}
}

void target_dl_stop(mt_string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		*sock << "104 ID";
		log_string(mt_string("Failed to stop download ID: ") + data, LOG_SEVERE);
	} else {
		curl_easy_setopt(it->get_handle(), CURLOPT_TIMEOUT, 1);
		if(it->get_status() != DOWNLOAD_INACTIVE) it->set_status(DOWNLOAD_PENDING);
		global_download_list.dump_to_file();
		log_string(mt_string("Stopped download ID: ") + data, LOG_DEBUG);
		*sock << "100 SUCCESS";
	}
}

void target_dl_up(mt_string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	switch (global_download_list.move_up(atoi(data.c_str()))) {
		case 0:
			log_string(mt_string("Moved download ID: ") + data + " upwards", LOG_DEBUG);
			*sock << "100 SUCCESS"; break;
		case -1:
			log_string(mt_string("Failed to move download ID: ") + data + " upwards", LOG_SEVERE);
			*sock << "104 ID"; break;
		case -2:
			log_string(mt_string("Failed to move download ID: ") + data + " upwards", LOG_SEVERE);
			*sock << "110 PERMISSION"; break;
	}
}

void target_dl_down(mt_string &data, tkSock *sock) {
	// implemented with move_up. using ++ to get the next download and move it up, in order to move the
	// current download down.
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end() || ++it == global_download_list.end()) {
		log_string(mt_string("Failed to move download ID: ") + data + " downwards.", LOG_SEVERE);
		*sock << "104 ID";
		return;
	}

	while(it->get_status() == DOWNLOAD_DELETED) {
		++it;
		if(it == global_download_list.end()) {
			log_string(mt_string("Failed to move download ID: ") + data + " downwards.", LOG_SEVERE);
			*sock << "104 ID";
			return;
		}
	}

	switch(global_download_list.move_up(atoi(data.c_str()))) {
		case 0:
			log_string(mt_string("Moved download ID: ") + data + " downwards", LOG_DEBUG);
			*sock << "100 SUCCESS"; break;
		case -1:
			log_string(mt_string("Failed to move download ID: ") + data + " downwards", LOG_SEVERE);
			*sock << "104 ID"; break;
		case -2:
			log_string(mt_string("Failed to move download ID: ") + data + " downwards", LOG_SEVERE);
			*sock << "110 PERMISSION"; break;
	}
}

void target_dl_activate(mt_string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		*sock << "104 ID";
		log_string(mt_string("Failed to activate download ID: ") + data, LOG_SEVERE);
	} else if(it->get_status() != DOWNLOAD_INACTIVE) {
		*sock << "106 ACTIVATE";
		log_string(mt_string("Failed to activate download ID: ") + data, LOG_WARNING);
	} else {
		it->set_status(DOWNLOAD_PENDING);
		if(!global_download_list.dump_to_file()) {
			it->set_status(DOWNLOAD_INACTIVE);
			log_string(mt_string("Failed to activate download ID: ") + data, LOG_SEVERE);
			*sock << "110 PERMISSION";
		} else {
			log_string(mt_string("Activated download ID: ") + data, LOG_DEBUG);
			*sock << "100 SUCCESS";
		}
	}
}

void target_dl_deactivate(mt_string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		*sock << "104 ID";
		log_string(mt_string("Failed to deactivate download ID: ") + data, LOG_SEVERE);
	} else {
		download_status old_status = it->get_status();
		if(it->get_status() == DOWNLOAD_RUNNING) {
			curl_easy_setopt(it->get_handle(), CURLOPT_TIMEOUT, 1);
			it->set_status(DOWNLOAD_INACTIVE, true);
		} else {
			it->set_status(DOWNLOAD_INACTIVE);
		}
		it->set_wait_seconds(0);
		it->set_size(0);
		it->set_downloaded_bytes(0);
		if(!global_download_list.dump_to_file()) {
			it->set_status(old_status);
			log_string(mt_string("Failed to deactivate download ID: ") + data, LOG_SEVERE);
			*sock << "110 PERMISSION";
		} else {
			log_string(mt_string("Deactivated download ID: ") + data, LOG_DEBUG);
			*sock << "100 SUCCESS";
		}
	}
}

void target_var(mt_string &data, tkSock *sock) {
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

void target_var_get(mt_string &data, tkSock *sock) {
	if(data == "mgmt_password") {
		*sock << "";
	} else {
		*sock << global_config.get_cfg_value(data);
	}
}

void target_var_set(mt_string &data, tkSock *sock) {
	if(!data.find('=')) {
		*sock << "101 PROTOCOL";
		return;
	}
	if(data.find("mgmt_password") == 0) {
		if(data.find(';') == mt_string::npos) {
			*sock << "101 PROTOCOL";
			return;
		}
		size_t pos1 = data.find('=') + 1;
		size_t pos2 = data.find(';', pos1);
		mt_string old_pw;
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
		mt_string identifier(data.substr(0, data.find('=')));
		trim_string(identifier);
		mt_string value(data.substr(data.find('=') + 1));
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

void target_file(mt_string &data, tkSock *sock) {
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

void target_file_del(mt_string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		log_string(mt_string("Failed to delete file ID: ") + data, LOG_WARNING);
		*sock << "104 ID";
		return;
	}
	if(it->get_output_file() == "" || it->get_status() == DOWNLOAD_RUNNING) {
		log_string(mt_string("Failed to delete file ID: ") + data, LOG_WARNING);
		*sock << "109 FILE";
		return;
	}
	if(remove(it->get_output_file().c_str()) == 0) {
		log_string(mt_string("Deleted file ID: ") + data, LOG_DEBUG);
		*sock << "100 SUCCESS";
		it->set_output_file("");
		global_download_list.dump_to_file();
	} else {
		*sock << "109 FILE";
	}
}

void target_file_getpath(mt_string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		log_string(mt_string("Failed to get path of file ID: ") + data, LOG_WARNING);
		*sock << "";
		return;
	}
	struct stat st;
	if(stat(it->get_output_file().c_str(), &st) != 0) {
		*sock << "";
		return;
	}
	*sock << it->get_output_file();
}

void target_file_getsize(mt_string &data, tkSock *sock) {
	download_container::iterator it = global_download_list.get_download_by_id(atoi(data.c_str()));
	if(it == global_download_list.end()) {
		log_string(mt_string("Failed to get size of file ID: ") + data, LOG_WARNING);
		*sock << "-1";
		return;
	}
	struct stat st;
	if(stat(it->get_output_file().c_str(), &st) != 0) {
		log_string(mt_string("Failed to get size of file ID: ") + data, LOG_WARNING);
		*sock << "-1";
		return;
	}
	*sock << int_to_string(st.st_size);

}

void target_router(mt_string &data, tkSock *sock) {
	if(data.find("LIST") == 0) {
		data = data.substr(4);
		trim_string(data);
		target_router_list(data, sock);
	} else if(data.find("SETMODEL") == 0) {
		data = data.substr(8);
		if(data.length() == 0 || !isspace(data[0])) {
			*sock << "101 PROTOCOL";
			return;
		}
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

void target_router_list(mt_string &data, tkSock *sock) {
    DIR *dp;
    struct dirent *ep;
    mt_string content;
    mt_string path = program_root + "reconnect/";
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
        content += ep->d_name;
        content += "\n";

    }
    (void) closedir (dp);

    content.erase(content.end() - 1);
    *sock << content;
}

void target_router_setmodel(mt_string &data, tkSock *sock) {
    struct dirent *de = NULL;
    DIR *d = NULL;
    mt_string dir = program_root + "reconnect/";
    d = opendir(dir.c_str());
    mt_string content;

    if(d == NULL) {
        log_string("Could not open reconnect script directory", LOG_SEVERE);
        *sock << "109 FILE";
        return;
    }
    while((de = readdir(d))) {
        content += de->d_name + '\n';
    }
    closedir(d);
    size_t pos;
    if((pos = content.find(data)) == mt_string::npos || content[pos - 1] != '\n' || content[pos + content.length()] != '\n') {
        log_string("Selected plugin not found", LOG_WARNING);
        *sock << "109 FILE";
        return;
    }

    if(global_router_config.set_cfg_value("router_model", data)) {
        log_string("Changed router model", LOG_DEBUG);
        *sock << "100 SUCCESS";
    } else {
        log_string("Unable to set router model", LOG_SEVERE);
        *sock << "110 PERMISSION";
    }
}

void target_router_set(mt_string &data, tkSock *sock) {
    size_t eqpos = data.find('=');
    if(eqpos == mt_string::npos) {
        *sock << "101 PROTOCOL";
        return;
    }
    mt_string identifier = data.substr(0, eqpos);
    mt_string variable = data.substr(eqpos);
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

void target_router_get(mt_string &data, tkSock *sock) {
    if(data == "router_password") {
        *sock << "";
        return;
    } else {
        *sock << global_router_config.get_cfg_value(data);
    }
}
