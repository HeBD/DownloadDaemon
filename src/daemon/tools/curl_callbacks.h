/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CURL_CALLBACKS_H_
#define CURL_CALLBACKS_H_

size_t write_file	(void *buffer, size_t size, size_t nmemb, void *userp);
int report_progress	(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
size_t parse_header	(void *ptr, size_t size, size_t nmemb, void *clientp);

#endif /*CURL_CALLBACKS_H_*/
