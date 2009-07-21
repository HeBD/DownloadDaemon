#include <vector>
#include "../dl/download.h"
#include "global_management.h"
using namespace std;

extern vector<download> global_download_list;

void tick_downloads() {
	while(true) {
		for(vector<download>::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
			if(it->wait_seconds > 0) {
				--it->wait_seconds;
			}
		}
		sleep(1);
	}
}