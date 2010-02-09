/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GLOBAL_MANAGEMENT_H_
#define GLOBAL_MANAGEMENT_H_

extern boost::shared_mutex once_per_sec_mutex;

/** Put things that need to be done once per second here
*/
void do_once_per_second();


#endif /*GLOBAL_MANAGEMENT_H_*/
