#ifndef REGEX_H
#define REGEX_H
#include <vector>
#include <string>
#include "slre.h"


/*! more advanced interface for regular expressions to individually compile and match regular expressions */
class RegEx {
public:
	RegEx();
	/*! compile a regular expression */
	bool compile(const std::string &regex);

	/*! match a previousely compiled regex to a text. the first version returns true if the regex matches.
		the second version also returns all matching results and the third version only returns the first match.
	*/
	bool match(const std::string &text);
	bool match(const std::string &text, std::vector<std::string> &result);
	bool match(const std::string &text, std::string &result);



	/*! 3 functions to simplify usage and compile/match in one go instead of seperate steps */
	static bool match(const std::string &text, const std::string &regex, std::vector<std::string> &result);
	static bool match(const std::string &text, const std::string &regex, std::string &result);
	static bool match(const std::string &text, const std::string &regex);
private:
	slre s;
	unsigned int num_brackets;
};

#endif
