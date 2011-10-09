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
#include <sys/stat.h>

#include <string>
#include <vector>
#include <cassert>
#include <iostream>

extern "C"
{
	#include "../../include/lua/src/lua.h"
	#include "../../include/lua/src/lualib.h"
	#include "../../include/lua/src/lauxlib.h"
}

using namespace std;

plugin_container::plugin_container()
{
	state = luaL_newstate();
	assert(state);
	luaL_openlibs(state);
}

plugin_container::~plugin_container() {
	unique_lock<recursive_mutex> lock(mx);
	lua_close(state);
}

plugin_output plugin_container::get_info(const std::string& info, p_info kind) {
	unique_lock<recursive_mutex> lock(mx);

	string host = info;
	if(kind == P_FILE) {
		try {
			size_t pos = info.rfind("lib") + 3;
			host = info.substr(pos, info.size() - 3 - pos);
		} catch (...) {
			log_string("plugin_container::get_info received invalid info: " + host + " which is of kind " + int_to_string((int)kind) + ". Please report this error.", LOG_ERR);
		}
	}

	std::pair<bool, plugin> ret = search_info_in_cache(host);
	plugin_output outp;
	if(ret.first) {
		// plugin is already in the cache - we just have to return the info.
		outp.allows_multiple = ret.second.allows_multiple;
		outp.allows_resumption = ret.second.allows_resumption;
		outp.offers_premium = ret.second.offers_premium;
		return outp;
	}

	// put the plugin in the cache and return info


	plugin_input inp;

	outp.allows_resumption = false;
	outp.allows_multiple = false;
	outp.offers_premium = false;

	if(handles.size() == 0) load_plugins();
	// Load the plugin function needed
	handleIter it = handles.find(host);

	if(host.empty() || it == handles.end()) {
		// no plugin found, using the default values
		outp.allows_resumption = true;
		outp.allows_multiple = true;
		return outp;
	}

	void (*plugin_getinfo)(plugin_input&, plugin_output&);
	plugin_getinfo = (void (*)(plugin_input&, plugin_output&))dlsym(it->second, "plugin_getinfo");

	try {
		string rhost = real_host(host);
		inp.premium_user = global_premium_config.get_cfg_value(rhost + "_user");
		inp.premium_password = global_premium_config.get_cfg_value(rhost + "_password");
		plugin_getinfo(inp, outp);

		plugin new_p;
		new_p.allows_multiple = outp.allows_multiple;
		new_p.allows_resumption = outp.allows_resumption;
		new_p.host = rhost;
		new_p.offers_premium = outp.offers_premium;
		plugin_cache.push_back(new_p);

	} catch(std::exception &e) { // nothing to do. It's just that the plugin won't be cached
	}
	return outp;
}


std::string plugin_container::real_host(std::string host) {
	unique_lock<recursive_mutex> lock(mx);
	if(host == "") {
		return "";
	}
	for(size_t i = 0; i < host.size(); ++i) host[i] = tolower(host[i]);

	if(handles.empty()) load_plugins();

	for(handleIter it = handles.begin(); it != handles.end(); ++it) {

		if(host.find(it->first) != string::npos) {
		   return it->first;
		}
	}
	return "";
}

void plugin_container::load_plugins() {
	unique_lock<recursive_mutex> lock(mx);
	string plugindir = program_root + "/plugins/";
	correct_path(plugindir);

	DIR *dp;
	struct dirent *ep;
	dp = opendir(plugindir.c_str());
	if (dp == NULL) {
		log_string("Could not open plugin directory.", LOG_ERR);
		return;
	}

	for(handleIter it = handles.begin(); it != handles.end(); ++it) {
		if(it->second) dlclose(it->second);
	}

	handles.clear();

	struct pstat st;

	while ((ep = readdir(dp))) {
		if(ep->d_name[0] == '.') {
			continue;
		}
		string current = plugindir + '/' + ep->d_name;

		if(pstat(current.c_str(), &st) != 0) continue;

		if(current.rfind(".lua") != current.size() - 4)
			continue;

		int r = luaL_dofile(state, current.c_str());
		if (r != 0) {
			log_string("Failed to load plugin " + current + ". Error: " + lua_tostring(state, -1), LOG_ERR);
			std::cerr << lua_tostring(state, -1);
			lua_pop(state, 1);
			continue;
		}
	}
	closedir(dp);

	vector<string> plugin_tbls;
	// now all plugins are read and executed. Now we can traverse through all the global objects
	// (see here: http://www.gamedev.net/topic/465208-lua---traversing-globals-table/ ).
	// make sure they are tables and check if everything is there and what is for what, etc..
	lua_getglobal(state, "_G");
	const int G = lua_gettop(state);
	lua_pushnil(state);
	while(lua_next(state, G)) {
		if (!lua_istable(state, -1)) {
			lua_pop(state, 1);
			continue;
		}
		lua_getfield(state, -1, "get_plugin_type");
		if (!lua_isfunction(state, -1)) {
			lua_pop(state, 2);
			continue;
		}
		lua_pop(state, 1);
		const char *key_c_str = lua_tostring(state, -2);
		if(key_c_str ) {
			plugin_tbls.push_back(key_c_str);
		}
		lua_pop(state, 1);
	}
	lua_pop(state, 1);
	//assert(lua_gettop(state) == 0);
}

const std::vector<plugin>& plugin_container::get_all_infos() {
	DIR *dp;
	struct dirent *ep;
	std::string plugindir = program_root + "/plugins/";
	correct_path(plugindir);
	plugindir += '/';
	dp = opendir(plugindir.c_str());
	if(!dp) {
		log_string("Could not open plugin directory!", LOG_ERR);
		return plugin_cache;
	}
	while((ep = readdir(dp))) {
		if(ep->d_name[0] == '.') {
			continue;
		}
		string current = ep->d_name;
		if(current.find("lib") == string::npos || current.find(".so") == string::npos) continue;
		current = plugindir + current;
		get_info(current, P_FILE);
	}
	return plugin_cache;
}

std::pair<bool, plugin> plugin_container::search_info_in_cache(const std::string& info) {
	for(vector<plugin>::iterator it = plugin_cache.begin(); it != plugin_cache.end(); ++it) {
		if(info.find(it->host) != std::string::npos) {
			return pair<bool, plugin>(true, *it);
		}
	}
	plugin p;
	return pair<bool, plugin>(false, p);
}

void* plugin_container::operator[](std::string plg) {
	unique_lock<recursive_mutex> lock(mx);
	for(size_t i = 0; i < plg.size(); ++i) plg[i] = tolower(plg[i]);
	if(handles.empty()) load_plugins();
	for(handleIter it = handles.begin(); it != handles.end(); ++it) {
		if(plg.find(it->first) != string::npos) {
			return it->second;
		}
	}
	return NULL;
}

void plugin_container::clear_cache() {
	unique_lock<recursive_mutex> lock(mx);
	plugin_cache.clear();
}
