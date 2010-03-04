#ifndef DDCLIENT_GUI_UPDATE_THREAD_H
#define DDCLIENT_GUI_UPDATE_THREAD_H

#include <QThread>
#include "ddclient_gui.h"


class update_thread : public QThread{

    public:
        /** Constructor
        *   @param parent Pointer to MainWindow
        */
        update_thread(ddclient_gui *parent);

        /** Defaultmethod to start Thread*/
        void run();

    private:
        ddclient_gui *parent;
};

#endif // DDCLIENT_GUI_UPDATE_THREAD_H
