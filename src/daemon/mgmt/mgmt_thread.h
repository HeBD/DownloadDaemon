/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef MGMT_THREAD_H_
#define MGMT_THREAD_H_

#include <vector>
#include "../dl/download.h"
#include "../../lib/cfgfile/cfgfile.h"
#include "../../lib/netpptk/netpptk.h"


void mgmt_thread_main();

void connection_handler(tkSock *sock);
void target_dl(std::string &data, tkSock *sock);
void target_var(std::string &data, tkSock *sock);
void target_file(std::string &data, tkSock *sock);
void target_router(std::string &data, tkSock *sock);

void target_dl_list(std::string &data, tkSock *sock);
void target_dl_add(std::string &data, tkSock *sock);
void target_dl_del(std::string &data, tkSock *sock);
void target_dl_stop(std::string &data, tkSock *sock);
void target_dl_up(std::string &data, tkSock *sock);
void target_dl_down(std::string &data, tkSock *sock);
void target_dl_activate(std::string &data, tkSock *sock);
void target_dl_deactivate(std::string &data, tkSock *sock);

void target_var_get(std::string &data, tkSock *sock);
void target_var_set(std::string &data, tkSock *sock);

void target_file_del(std::string &data, tkSock *sock);
void target_file_getpath(std::string &data, tkSock *sock);
void target_file_getsize(std::string &data, tkSock *sock);

void target_router_list(std::string &data, tkSock *sock);
void target_router_setmodel(std::string &data, tkSock *sock);
void target_router_set(std::string &data, tkSock *sock);
void target_router_get(std::string &data, tkSock *sock);

#endif /*MGMT_THREAD_H_*/
