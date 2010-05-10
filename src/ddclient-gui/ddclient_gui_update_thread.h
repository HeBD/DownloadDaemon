/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_GUI_UPDATE_THREAD_H
#define DDCLIENT_GUI_UPDATE_THREAD_H

#include <QThread>
#include "ddclient_gui.h"


class update_thread : public QThread{

    public:
        /** Constructor
        *   @param parent Pointer to MainWindow
        *   @param interval Update Interval
        */
        update_thread(ddclient_gui *parent, int interval);

        /** Defaultmethod to start Thread*/
        void run();

        /** Setter for Update Interval
        *    @param interval Interval to set
        */
        void set_update_interval(int interval);

        /** Toggles updating */
        void toggle_updating();


    private:
        ddclient_gui *parent;
        bool told;
        bool update;
        int interval;
};

#endif // DDCLIENT_GUI_UPDATE_THREAD_H
