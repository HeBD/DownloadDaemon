/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "client_exception.h"

client_exception::client_exception(std::string message = "") : message(message){
}


client_exception::~client_exception() throw(){
}


const char* client_exception::what() const throw(){
	return message.c_str();
}
