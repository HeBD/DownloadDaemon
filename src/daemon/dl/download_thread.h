/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

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
