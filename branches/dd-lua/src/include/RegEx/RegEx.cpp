#include <vector>
#include <string>

#include "RegEx.h"

using std::vector;
using std::string;
using std::size_t;

RegEx::RegEx() : num_brackets(0) {
}

bool RegEx::compile(const std::string &ex) {
	num_brackets = 0;
	if (slre_compile(&s, ex.c_str())) {
		for (size_t i = 0; i < ex.size(); ++i)
			if (ex[i] == '(') ++num_brackets;
	}

	return false;
}

bool RegEx::match(const std::string &text) {
	return slre_match(&s, text.c_str(), text.size(), NULL);
}

bool RegEx::match(const std::string &text, vector<string> &result) {
	struct cap* captures = new cap[num_brackets + 1];  // Number of braket pairs + 1
	if (slre_match(&s, text.c_str(), text.size(), captures)) {
		for (unsigned int i = 0; i < num_brackets + 1; ++i)
			result.push_back(std::string(captures[i].ptr, captures[i].len));
		return true;
	}
	return false;
}

bool RegEx::match(const std::string &text, std::string &result) {
	result.clear();
	vector<string> res;
	bool ret = match(text, res);
	if (res.size()) result = res[0];
	return ret;
}

bool RegEx::match(const std::string &text, const std::string &ex) {
	RegEx r;
	if (!r.compile(ex)) return false;
	return r.match(text);
}

bool RegEx::match(const std::string &text, const std::string &ex, std::vector<std::string> &result) {
	RegEx r;
	result.clear();
	if (!r.compile(ex)) return false;
	return r.match(text, result);
}

bool RegEx::match(const std::string &text, const std::string &ex, std::string &result) {
	result.clear();
	vector<string> res;
	bool ret = match(text, ex, res);
	if (res.size()) result = res[0];
	return ret;
}

#ifdef REGEX_TESTING
#include <iostream>
#include <cstdlib>
int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cerr << "usage: " << argv[0] << " regex text" << std::endl;
		std::exit(1);
	}
	
	std::vector<std::string> result;
	regex::match(argv[2], argv[1], result);
	for (unsigned int i = 0; i < result.size(); ++i) {
		std::cout << "match: " << result[i] << std::endl;
	}
}
#endif

