#ifndef DOWNLOAD_CONTAINER_H_INCLUDED
#define DOWNLOAD_CONTAINER_H_INCLUDED

#include "download.h"
#include <string>
#include <vector>

class download_container {
public:
	typedef std::vector<download>::iterator iterator;

	download_container(const char* filename);

	download_container() {}

	bool from_file(const char* filename);

	iterator get_download_by_id(int id);

	iterator begin() { return download_list.begin(); }
	iterator end() { return download_list.end(); }
	bool empty() { return download_list.empty(); }
	bool push_back(download &dl) { download_list.push_back(dl); return dump_to_file(); }
	bool pop_back() { download_list.pop_back(); return dump_to_file(); }
	bool erase(download_container::iterator it) { download_list.erase(it); return dump_to_file(); }

	int running_downloads();
	int total_downloads();

	bool dump_to_file();

private:

	std::string list_file;
	std::vector<download> download_list;
};



#endif // DOWNLOAD_CONTAINER_H_INCLUDED
