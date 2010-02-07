#ifndef CAPTCHA_H_INCLUDED
#define CAPTCHA_H_INCLUDED

#include <string>

#ifdef IS_PLUGIN
#include "plugin_helpers.h"

class captcha {
public:
	/** captcha-decoder constructor
	*	@param img Image file in binary
	*/
	captcha(std::string img) : image(img) {}

	/** do the actual decoding work
	*	@param gocr_options String which specifies the options to pass to gocr
	*	@param img_type type of the image file ("png", "jpg", "gif", etc)
	*	@param use_db should a gocr-letter-database be used for decrypting?
	*	@param keep_whitespaces should whitespaces be stripped out of the resulting text?
	*	@returns the captcha as clear text
	*/
	std::string process_image(std::string gocr_options, std::string img_type, bool use_db = false, bool keep_whitespaces = false);

private:
	std::string image;

};

#endif

class captcha_exception {};

#endif // CAPTCHA_H_INCLUDED
