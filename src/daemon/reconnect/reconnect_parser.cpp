#include "reconnect_parser.h"
#include "../tools/helperfunctions.h"
#include <cstdlib>
using namespace std;

reconnect::reconnect(const std::string &path_p, const std::string &host_p, const std::string &user_p, const std::string &pass_p)
	: path(path_p), host(host_p), user(user_p), pass(pass_p) {
	handle = curl_easy_init();
	variables.insert(pair<string, string>("%user%", user));
	variables.insert(pair<string, string>("%pass%", pass));
	variables.insert(pair<string, string>("%routerip%", host));
}

reconnect::~reconnect() {
	curl_easy_cleanup(handle);
}


bool reconnect::do_reconnect() {
	file.open(path.c_str());
	if(!file.good() || !file.is_open()) {
		log_string("Unable to open reconnect script", LOG_WARNING);
		return false;
	}

	while(!curr_line.empty() || getline(file, curr_line)) {
		trim_string(curr_line);
		exec_next();
	}
	return reconnect_success;
}


bool reconnect::exec_next() {
	if(curr_line.find("[[[") != 0) {
		return false;
	}
	if(curr_line.find("[[[STEP") == 0) {
		curr_line = curr_line.substr(7);
		step();
	} else if(curr_line.find("[[[REQUEST") == 0) {
		curr_line = curr_line.substr(10);
		request();
	} else if(curr_line.find("[[[WAIT") == 0) {
		curr_line = curr_line.substr(7);
		wait();
	} else if(curr_line.find("[[[DEFINE") == 0) {
		curr_line = curr_line.substr(9);
		define();
	}else {
		if(curr_line.find("]]]") != string::npos) {
			curr_line = curr_line.substr(curr_line.find("]]]") + 3);
		} else {
			reconnect_success = false;
			return false;
		}
	}

	return true;
}

void reconnect::step() {
	if(curr_line.find("]]]") == string::npos) {
		reconnect_success = false;
		return;
	}

	curr_line = curr_line.substr(curr_line.find("]]]") + 3);

	while(!curr_line.empty() || getline(file, curr_line)) {
		trim_string(curr_line);
		if(curr_line.empty()) {
			continue;
		}

		std::string cmd = curr_line.substr(0, curr_line.find("[[["));
		trim_string(cmd);
		if(cmd.length() > 0) {
			// stuff that stands in [[[STEP]]] but not in [[[REQEST]]], saved in cmd
		}
		curr_line = curr_line.substr(curr_line.find("[[["));
		if(curr_line.find("[[[/STEP") == 0) {
			if(curr_line.find("]]]") != string::npos) {
				curr_line = curr_line.substr(curr_line.find("]]]") + 3);
				trim_string(curr_line);
			} else {
				reconnect_success = false;
				curr_line.clear();
			}
			trim_string(curr_line);
			return;
		}
		trim_string(curr_line);
		exec_next();
	}
}

void reconnect::request() {
	if(curr_line.find("]]]") != 0) {
		reconnect_success = false;
		return;
	}
	curr_line = curr_line.substr(curr_line.find("]]]") + 3);
	trim_string(curr_line);
	curl_easy_reset(handle);
	std::string resultstr;
	curl_easy_setopt(handle, CURLOPT_COOKIEFILE, "");
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, reconnect::write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &resultstr);
	curl_easy_setopt(handle, CURLOPT_URL, string("http://" + host).c_str());
	struct curl_slist *header = NULL;

	while(!curr_line.empty() || getline(file, curr_line)) {
		trim_string(curr_line);
		if(curr_line.empty()) continue;
		std::string cmd;
		cmd = curr_line.substr(0, curr_line.find("[[["));
		if(curr_line.find("[[[") != string::npos) {
			curr_line = curr_line.substr(curr_line.find("[[["));
		} else {
			curr_line.clear();
		}

		if(cmd.empty() && curr_line.find("[[[/REQUEST]]]") == 0) {
			if(header != NULL)
				curl_easy_setopt(handle, CURLOPT_HTTPHEADER, header);

			if(!curr_post_data.empty()) {
				curl_easy_setopt(handle, CURLOPT_POST, 1);
				curl_easy_setopt(handle, CURLOPT_POSTFIELDS, curr_post_data.c_str());
			}
			curl_easy_perform(handle);
			if(header != NULL)
				curl_slist_free_all(header);
			curr_post_data.clear();
			curr_line = curr_line.substr(14);
			return;
		}
		else if(cmd.empty()) {
			exec_next();
		} else {
			substitute_vars(cmd);
			if(cmd.empty()) continue;
			if(cmd.find("%basicauth%") != string::npos) {
				curl_easy_setopt(handle, CURLOPT_USERPWD, string(user + ':' + pass).c_str());
				cmd = "";
				continue;
			}

			if(cmd.find("GET") == 0 || cmd.find("POST") == 0) {
				cmd = cmd.substr(4);
				trim_string(cmd);
				cmd = cmd.substr(0, cmd.find_first_of(" \t\n"));
				cmd = "http://" + host + cmd;
				curl_easy_setopt(handle, CURLOPT_URL, cmd.c_str());
				continue;
			}

			if(cmd.find(':') == string::npos) {
				curr_post_data = cmd;
				continue;
			}
			header = curl_slist_append(header, cmd.c_str());
		}
		trim_string(curr_line);
	}
}

void reconnect::wait() {
	std::string cmd = curr_line.substr(0, curr_line.find("]]]"));
	trim_string(cmd);
	if(cmd.empty()) return;
	size_t n;
	if((n = cmd.find("seconds")) != string::npos) {
		n = cmd.find('=', n);
		std::string num_secs;
		bool num_found = false;
		for(size_t i = n; i < cmd.length() && !num_found; ++i) {
			if(isdigit(cmd[i])) {
				num_found = true;
				num_secs += cmd[i];
			} else if(num_found) {
				break;
			}
		}
		int secs = atoi(num_secs.c_str());
		sleep(secs);
	}
	if(curr_line.find("]]]")) {
		curr_line = curr_line.substr(curr_line.find("]]]") + 3);
		trim_string(curr_line);
	} else {
		curr_line.clear();
	}
}

void reconnect::define() {
	string cmd = curr_line.substr(0, curr_line.find("]]]"));
	trim_string(cmd);
	if(cmd.empty()) return;
	cmd += "="; // so we don't get off-by-one error
	for(size_t i = cmd.find('='); cmd.find('=', i + 1) != string::npos;) {
		size_t j = i;
		while(isspace(cmd[--j]));
		while(!isspace(cmd[j--]) && j != 0);
		string varname = cmd.substr(j, i);
		trim_string(varname);
		varname += "%";
		varname.insert(0, "%");

		string value;
		size_t start = cmd.find('"', i) + 1;
		j = start + 1;
		while(cmd.length() > j++) {
			if(cmd[j] == '\"' && cmd[j-1] != '\\') {
				break;
			}
		}
		value = cmd.substr(start, j - start);
		variables.insert(pair<string, string>(varname, value));
	}

	if(curr_line.find("]]]") != string::npos) {
		curr_line.substr(curr_line.find("]]]") + 3);
	} else {
		curr_line.clear();
	}
}


size_t reconnect::write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

void reconnect::substitute_vars(std::string& str) {
	size_t n;

	for(std::map<std::string, std::string>::iterator it = variables.begin(); it != variables.end(); ++it) {
		if((n = str.find(it->first)) != string::npos) {
			str.replace(n, it->first.length(), it->second);

			while(str[--n] == '%') {
				str.erase(n, 1);
			}

			n += it->second.length() + 1;
			while(str.length() > n && str[n] == '%') {
				str.erase(n, 1);
			}
		}
	}
	if(str.find("%Set-Cookie%") != string::npos) {
		str.clear();
	}

}
