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

download_container global_download_list;
cfgfile global_config;
cfgfile global_router_config;
std::string program_root;

#endif /*GLOBAL_H_*/
