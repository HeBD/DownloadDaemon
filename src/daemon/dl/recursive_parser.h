#ifndef RECURSIVE_PARSER_H_INCLUDED
#define RECURSIVE_PARSER_H_INCLUDED

#include <string>
#include <vector>

class recursive_parser {
public:
	recursive_parser(std::string url);
	void add_to_list();

	static size_t to_string_callback(void *buffer, size_t size, size_t nmemb, void *userp);

private:
	void deep_parse(std::string url);
	std::vector<std::string> parse_list(std::string list);
	std::string get_list(std::string url);

	std::string folder_url;



};

#endif // RECURSIVE_PARSER_H_INCLUDED
