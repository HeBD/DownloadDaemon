/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef PLUGIN_CONTAINER_H_INCLUDED
#define PLUGIN_CONTAINER_H_INCLUDED

#include <string>
#include <vector>

#include "download.h"

#ifndef USE_STD_THREAD
#include <boost/thread.hpp>
namespace std {
	using namespace boost;
}
#else
#include <thread>
#endif

struct plugin {
	std::string host;
	std::string plugin_file;

	bool allows_multiple;
	bool allows_resumption;
	bool offers_premium;
};


class plugin_container {
public:
	enum p_info { P_HOST, P_FILE };

	plugin_output get_info(const std::string& info, p_info kind);
	const std::vector<plugin>& get_all_infos();

	void clear_cache();


private:
	std::string plugin_file_from_host(std::string host);
	std::pair<bool, plugin> search_info_in_cache(const std::string& info, p_info kind);

	std::vector<plugin> plugin_cache;
	mutable std::recursive_mutex mx;
};

#endif // PLUGIN_CONTAINER_H_INCLUDED
