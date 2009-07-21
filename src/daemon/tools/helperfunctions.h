#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_

#include <string>

enum LOG_LEVEL { LOG_OFF = 0, LOG_SEVERE, LOG_WARNING, LOG_DEBUG };

/** Convert a string to an int
* @param str string to convert
* @return result
*/
int string_to_int(std::string str);

std::string int_to_string(int i);

/** Remove whitespaces from beginning and end of a string
* @param str string to process
*/
void trim_string(std::string &str);

/** Validation of a given URL to check if it's valid
* @param url Url to check
* @return true if valid
*/
bool validate_url(std::string &url);

/** log a string with a specified log-level
* @param logstr String to write to the log
* @param level Log-level of the string - the higher, the more likely the user will see it (possible: LOG_OFF, LOG_SEVERE, LOG_WARNING, LOG_DEBUG)
*/
void log_string(const std::string logstr, LOG_LEVEL level);

/** replaces all occurences of a string with another string
* @param searchIn String in which stuff has to be replaced
* @param searchFor String with stuff to replace
* @param ReplaceWith String that should be inserted instead
*/
void replace_all(std::string& searchIn, std::string searchFor, std::string ReplaceWith);

#endif /*HELPERFUNCTIONS_H_*/
