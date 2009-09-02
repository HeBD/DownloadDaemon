#ifndef DOWNLOAD_THREAD_H_
#define DOWNLOAD_THREAD_H_

#include "download.h"
#include "download_container.h"
#include <vector>

void download_thread_main();
int get_running_count();
//download_container::iterator get_next_downloadable();
size_t write_file(void *buffer, size_t size, size_t nmemb, void *userp);
//download_container::iterator get_download_by_id(int id);
void download_thread(int id);

#endif /*DOWNLOAD_THREAD_H_*/
