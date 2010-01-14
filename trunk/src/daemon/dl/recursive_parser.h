#ifndef RECURSIVE_PARSER_H_INCLUDED
#define RECURSIVE_PARSER_H_INCLUDED

#include <string>
#include <vector>

class recursive_parser {
public:
	/** default constructor
	*   @param url URL to the folder
	*/
	recursive_parser(std::string url);

	/** checks out the folder recursively and adds all downloads to global_download_list */
	void add_to_list();

	/** callback for curl, needs to be static */
	static size_t to_string_callback(void *buffer, size_t size, size_t nmemb, void *userp);

private:
	/** does the real work
	*   @param url URL to the folder to parse (don't use member so we can call recursively
	*/
	void deep_parse(std::string url);

	/** parses the list returned from a curl-download on a ftp folder
	*   @param list the list as it's returned from curl
	*   @returns a vector of filenames. if it's a directory, he last letter is a /
	*/
	std::vector<std::string> parse_list(std::string list);

	/** download the directory index
	*   @param url URL to the directory
	*	@returns the list
	*/
	std::string get_list(std::string url);

	std::string folder_url;



};

#endif // RECURSIVE_PARSER_H_INCLUDED
