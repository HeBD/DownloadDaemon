/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "language.h"


language::language() : lang("English"){
}


void language::set_working_dir(std::string working_dir){
	boost::mutex::scoped_lock lock(mx);
	this->working_dir = working_dir;
}


bool language::set_language(std::string lang){
	boost::mutex::scoped_lock lock(mx);
	texts.clear();

	// einlesen der datei
	if(lang != "English"){ // default language is english
		size_t found;

		if(working_dir == "")
			return false;


		std::string file_name = working_dir + lang;
		std::ifstream ifs(file_name.c_str(), std::fstream::in); // open file

		if(ifs.good()){ // file successfully opened
			char line[512];

			while((!ifs.eof())){ // loop through lines
				ifs.getline(line, 512);

				std::string english(line);
				std::string lang_string;

				found = english.find("->"); // english and language string are separated by ->

				if (found != std::string::npos && english[0] != '#'){ // split line in english and language string and insert it
					lang_string = english.substr(found+2);
					english = english.substr(0, found);

					texts.insert(std::pair<std::string, std::string>(english, lang_string));
				}
			}

			ifs.close(); // close file
		}else // error at opening
			return false;

	}

	this->lang = lang;
	return true;
}


std::string language::operator[](std::string index){
	boost::mutex::scoped_lock lock(mx);

	if(lang == "English") // default language is English
		return index;

	if(texts.find(index) == texts.end()) // if index can't be found return index
		return index;

	else
		return texts[index];
}
