/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_

#include <string>

enum LOG_LEVEL { LOG_OFF = 0, LOG_SEVERE, LOG_WARNING, LOG_DEBUG };

/** Convert a string to an int
* @param str string to convert
* @returns result
*/
int string_to_int(std::string str);

/** Conversion functions for a few types */
std::string int_to_string(int i);
std::string long_to_string(long i);
long string_to_long(std::string str);

/** Remove whitespaces from beginning and end of a string
* @param str string to process
*/
void trim_string(std::string &str);

/** Validation of a given URL to check if it's valid
* @param url Url to check
* @returns true if valid
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

/** Dumps the current internal download list (in vector global_download_list) to the download list file on the harddisk.
* @returns ture on success
*/
bool dump_list_to_file();

/** Checks if a given identifier is a valid configuration variable or not
* @param variable Identifier to check
* @returns True if it's valid, false if not
*/
bool variable_is_valid(std::string &variable);

/** Checks if a given idetifier is a valid router config variable or not
* @param variable Identifier to check
* @returns true if it's valid, false if not
*/
bool router_variable_is_valid(std::string &variable);

/** Checks if a path is relative or absolute and if it's relative, add the program_root in the beginning
* @param path Path to check/modify
*/
void correct_path(std::string &path);

#endif /*HELPERFUNCTIONS_H_*/
