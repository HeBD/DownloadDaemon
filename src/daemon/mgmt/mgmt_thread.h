#ifndef MGMT_THREAD_H_
#define MGMT_THREAD_H_

#include <vector>
#include "../dl/download.h"
#include "../../lib/cfgfile/cfgfile.h"
#include "../../lib/netpptk/netpptk.h"


void mgmt_thread_main();
void connection_handler(tkSock *sock);

bool add(std::string data);
bool del(std::string data);
bool get(std::string data, tkSock &sock);
bool set(std::string data);
unsigned int get_next_id();
bool dump_list_to_file();

#endif /*MGMT_THREAD_H_*/
