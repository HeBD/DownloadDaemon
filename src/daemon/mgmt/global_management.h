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

/** This function ticks the download wait-times down
*/
void tick_downloads();

/** Handles reconnecting configuration and policies
*/
void reconnect();

#endif /*GLOBAL_MANAGEMENT_H_*/
