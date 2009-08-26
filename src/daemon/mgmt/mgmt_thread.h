#ifndef MGMT_THREAD_H_
#define MGMT_THREAD_H_

#include <vector>
#include "../dl/download.h"
#include "../../lib/cfgfile/cfgfile.h"
#include "../../lib/netpptk/netpptk.h"


void mgmt_thread_main();

void connection_handler(tkSock *sock);
void target_dl(mt_string &data, tkSock *sock);
void target_var(mt_string &data, tkSock *sock);
void target_file(mt_string &data, tkSock *sock);

void target_dl_list(mt_string &data, tkSock *sock);
void target_dl_add(mt_string &data, tkSock *sock);
void target_dl_del(mt_string &data, tkSock *sock);
void target_dl_stop(mt_string &data, tkSock *sock);
void target_dl_up(mt_string &data, tkSock *sock);
void target_dl_down(mt_string &data, tkSock *sock);
void target_dl_activate(mt_string &data, tkSock *sock);
void target_dl_deactivate(mt_string &data, tkSock *sock);

void target_var_get(mt_string &data, tkSock *sock);
void target_var_set(mt_string &data, tkSock *sock);

void target_file(mt_string &data, tkSock *sock);
void target_file_del(mt_string &data, tkSock *sock);
void target_file_getpath(mt_string &data, tkSock *sock);
void target_file_getsize(mt_string &data, tkSock *sock);

void target_router(mt_string &data, tkSock *sock);
void target_router_list(mt_string &data, tkSock *sock);
void target_router_setmodel(mt_string &data, tkSock *sock);
void target_router_set(mt_string &data, tkSock *sock);
void target_router_get(mt_string &data, tkSock *sock);

#endif /*MGMT_THREAD_H_*/
