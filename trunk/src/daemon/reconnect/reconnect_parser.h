#ifndef RECONNECT_PARSER_H_INCLUDED
#define RECONNECT_PARSER_H_INCLUDED

#include <string>
#include <fstream>
#include <map>
#include <curl/curl.h>

class reconnect {
public:
	reconnect(const std::string &path_p, const std::string &host_p, const std::string &user_p, const std::string &pass_p);

	~reconnect();

	bool do_reconnect();




private:
	std::map<std::string, std::string> variables;
	std::string path, host, user, pass, curr_line;
	std::ifstream file;
	CURL* handle;
	bool reconnect_success;

	bool exec_next();
	void step();
	void request();
	void wait();
	void define();

	static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);

	void substitute_vars(std::string& str);
	std::string curr_post_data;

};

#endif // RECONNECT_PARSER_H_INCLUDED
