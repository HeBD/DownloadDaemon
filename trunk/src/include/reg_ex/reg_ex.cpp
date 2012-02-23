#include <vector>
#include <string>
#include <cstring>

#include "reg_ex.h"

using std::vector;
using std::string;
using std::size_t;

reg_ex::reg_ex() : num_brackets(0) {
}

bool reg_ex::compile(const std::string &ex) {
	num_brackets = 0;
	if (slre_compile(&s, ex.c_str())) {
		for (size_t i = 0; i < ex.size(); ++i)
			if (ex[i] == '(') ++num_brackets;
        return true;
	}

	return false;
}

bool reg_ex::match(const std::string &text) {
	return slre_match(&s, text.c_str(), text.size(), NULL);
}

bool reg_ex::match(const std::string &text, vector<string> &result) {
    struct cap* captures = new cap[num_brackets + 1];  // Number of braket pairs + 1
    std::memset(captures, 0, sizeof(cap) * num_brackets + 1);

    if (slre_match(&s, text.c_str(), text.size(), captures)) {
        for (unsigned int i = 0; i < num_brackets + 1; ++i) {
            if (captures[i].ptr == NULL || captures[i].len == 0) break;
			result.push_back(std::string(captures[i].ptr, captures[i].len));
        }

        delete [] captures;
		return true;
	}
    delete [] captures;
	return false;
}

bool reg_ex::match(const std::string &text, std::string &result) {
	result.clear();
	vector<string> res;
	bool ret = match(text, res);
	if (res.size()) result = res[0];
	return ret;
}

bool reg_ex::match(const std::string &text, const std::string &ex) {
	reg_ex r;
	if (!r.compile(ex)) return false;
	return r.match(text);
}

bool reg_ex::match(const std::string &text, const std::string &ex, std::vector<std::string> &result) {
	reg_ex r;
	result.clear();
	if (!r.compile(ex)) return false;
	return r.match(text, result);
}

bool reg_ex::match(const std::string &text, const std::string &ex, std::string &result) {
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
	reg_ex r;
	if (!r.compile(argv[1])) {
		std::cout << "failed to compile regex" << std::endl;
		return 1;
	}
	std::vector<std::string> result;
	std::cout << "match: " << r.match(argv[2], result) << std::endl;
	std::cout << "results: " << std::endl;
	for (unsigned int i = 0; i < result.size(); ++i) {
		std::cout << "match: " << result[i] << std::endl;
	}
}
#endif

