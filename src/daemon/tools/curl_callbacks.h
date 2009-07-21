#ifndef CURL_CALLBACKS_H_
#define CURL_CALLBACKS_H_

size_t write_file(void *buffer, size_t size, size_t nmemb, void *userp);
int report_progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

#endif /*CURL_CALLBACKS_H_*/
