/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <vector>
#include "../lib/cfgfile/cfgfile.h"
#include "dl/download_container.h"

// The downloadcontainer is just needed everywhere in the program, so let's make it global
download_container global_download_list;

// configuration variables are also used a lot, so global too
cfgfile global_config;
cfgfile global_router_config;
cfgfile global_premium_config;

// same goes for the program root, which is needed for a lot of path-calculation
std::string program_root;

// the environment variables are also needed a lot for path calculations
char** env_vars;

#endif /*GLOBAL_H_*/
