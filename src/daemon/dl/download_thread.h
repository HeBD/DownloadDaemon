#ifndef DOWNLOAD_THREAD_H_
#define DOWNLOAD_THREAD_H_

#include "download.h"
#include "download_container.h"
#include <vector>

void download_thread_main();
int get_running_count();
size_t write_file(void *buffer, size_t size, size_t nmemb, void *userp);
void download_thread(int id);

#endif /*DOWNLOAD_THREAD_H_*/
