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
