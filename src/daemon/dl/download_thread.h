#ifndef DOWNLOAD_THREAD_H_
#define DOWNLOAD_THREAD_H_

#include "download.h"
#include <vector>

void download_thread_main();
int get_running_count();
std::vector<download>::iterator get_next_downloadable();
size_t write_file(void *buffer, size_t size, size_t nmemb, void *userp);
std::vector<download>::iterator get_download_by_id(int id);
void download_thread(std::vector<download>::iterator download);

#endif /*DOWNLOAD_THREAD_H_*/
