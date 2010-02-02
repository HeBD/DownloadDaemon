#ifndef RECONNECT_PARSER_H_INCLUDED
#define RECONNECT_PARSER_H_INCLUDED

#include <string>
#include <fstream>
#include <map>
#include <curl/curl.h>

class reconnect {
public:
	/** initializes the reconnect-object. this does not execute a reconnect yet
	 *	@param path_p path to the reconnect-script
	 *	@param host_p IP-address of the router
	 *	@param user_p username needed to log into the router
	 *	@param pass_p password for router login
	 */
	reconnect(const std::string &path_p, const std::string &host_p, const std::string &user_p, const std::string &pass_p);

	~reconnect();

	/** get your current public ip-address by querying http://www.whatismyip.com/automation/n09230945.asp
	 *	@returns the ip-address
	 */
	static std::string get_current_ip();

	/** executes the reconnect specified in the constructor
	 *	@returns true on success, false if we still have the same IP
	 */
	bool do_reconnect();

private:
	std::map<std::string, std::string> variables;
	std::string path, host, user, pass, curr_line;
	std::ifstream file;
	CURL* handle;

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
