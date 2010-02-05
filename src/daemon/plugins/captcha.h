#ifndef CAPTCHA_H_INCLUDED
#define CAPTCHA_H_INCLUDED

#include <string>

#ifdef IS_PLUGIN
#include "plugin_helpers.h"

class captcha {
public:
	captcha(std::string img) : image(img) {}

	std::string process_image(std::string gocr_options, std::string img_type, bool use_db = false, bool keep_whitespaces = false);

private:
	std::string image;

};

#endif

class captcha_exception {};

#endif // CAPTCHA_H_INCLUDED
