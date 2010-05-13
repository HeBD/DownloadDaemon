/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "plugin_container.h"
#include "../global.h"
#include "../tools/helperfunctions.h"
#include "download.h"

#include <dirent.h>
#include <dlfcn.h>

#include <string>
#include <vector>

using namespace std;

plugin_output plugin_container::get_info(const std::string& info, p_info kind) {
	unique_lock<recursive_mutex> lock(mx);
	std::pair<bool, plugin> ret = search_info_in_cache(info, kind);
	plugin_output outp;
	if(ret.first) {
		// plugin is already in the cache - we just have to return the info.
		outp.allows_multiple = ret.second.allows_multiple;
		outp.allows_resumption = ret.second.allows_resumption;
		outp.offers_premium = ret.second.offers_premium;
		return outp;
	}

	// put the plugin in the cache and return info
	string plugin_file = info;
	if(kind == P_HOST) {
		plugin_file = plugin_file_from_host(info);
	}

	plugin_input inp;

	outp.allows_resumption = false;
	outp.allows_multiple = false;
	outp.offers_premium = false;

	if(plugin_file.empty()) {
		outp.allows_multiple = true;
		outp.allows_resumption = true;
		return outp;
	}

	// Load the plugin function needed
	void* l_handle = dlopen(plugin_file.c_str(), RTLD_LAZY | RTLD_LOCAL);
	if (!l_handle) {
		log_string(std::string("Unable to open library file: ") + dlerror() + '/' + plugin_file, LOG_ERR);
		return outp;
	}

	dlerror();	// Clear any existing error

	void (*plugin_getinfo)(plugin_input&, plugin_output&);
	plugin_getinfo = (void (*)(plugin_input&, plugin_output&))dlsym(l_handle, "plugin_getinfo");

	const char* l_error;
	if ((l_error = dlerror()) != NULL)  {
		log_string(std::string("Unable to get plugin information: ") + l_error, LOG_ERR);
		dlclose(l_handle);
		return outp;
	}

	try {
		string real_host = plugin_file;
		real_host = real_host.substr(real_host.find_last_of("/"));
		real_host = real_host.substr(real_host.find("lib") + 3);
		real_host = real_host.substr(0, real_host.find(".so"));

		inp.premium_user = global_premium_config.get_cfg_value(real_host + "_user");
		inp.premium_password = global_premium_config.get_cfg_value(real_host + "_password");
		plugin_getinfo(inp, outp);
		dlclose(l_handle);

		plugin new_p;
		new_p.allows_multiple = outp.allows_multiple;
		new_p.allows_resumption = outp.allows_resumption;
		new_p.plugin_file = plugin_file;
		plugin_file = plugin_file.substr(plugin_file.find_last_of("/\\"));
		plugin_file = plugin_file.substr(plugin_file.find("lib") + 3);
		plugin_file = plugin_file.substr(0, plugin_file.find(".so"));
		new_p.host = plugin_file;
		new_p.offers_premium = outp.offers_premium;
		plugin_cache.push_back(new_p);

	} catch(std::exception &e) { // nothing to do. It's just that the plugin won't be cached
	}
	return outp;
}


std::string plugin_container::plugin_file_from_host(std::string host) {
	unique_lock<recursive_mutex> lock(mx);
	if(host == "") {
		return "";
	}
	for(size_t i = 0; i < host.size(); ++i) host[i] = tolower(host[i]);

	std::string plugindir = program_root + "/plugins/";
	correct_path(plugindir);
	plugindir += '/';

	DIR *dp;
	struct dirent *ep;
	dp = opendir(plugindir.c_str());
	if (dp == NULL) {
		log_string("Could not open plugin directory!", LOG_ERR);
		return "";
	}

	std::string result;
	std::string current;
	while ((ep = readdir(dp))) {
		if(ep->d_name[0] == '.') {
			continue;
		}
		current = ep->d_name;
		for(size_t i = 0; i < current.size(); ++i) current[i] = tolower(current[i]);
		if(current.find("lib") != 0) continue;
		current = current.substr(3);
		if(current.find(".so") == string::npos) continue;
		current = current.substr(0, current.find(".so"));
		if(host.find(current) != string::npos) {
			result = plugindir + ep->d_name;
			break;
		}
	}

	closedir(dp);

	return result;

}

std::pair<bool, plugin> plugin_container::search_info_in_cache(const std::string& info, p_info kind) {
	for(vector<plugin>::iterator it = plugin_cache.begin(); it != plugin_cache.end(); ++it) {
		if(kind == P_HOST && info.find(it->host) != std::string::npos) {
			return make_pair<bool, plugin>(true, *it);
		} else if(kind == P_FILE && info == it->plugin_file) {
			return make_pair<bool, plugin>(true, *it);
		}
	}
	plugin p;
	return make_pair<bool, plugin>(false, p);
}

void plugin_container::clear_cache() {
	unique_lock<recursive_mutex> lock(mx);
	plugin_cache.clear();
}
