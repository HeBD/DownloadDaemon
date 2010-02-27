#include "ddclient_gui.h"
#include <QtGui/QStatusBar>

ddclient_gui::ddclient_gui() : QMainWindow(NULL){
    setWindowTitle("DownloadDaemon Client GUI");
    this->resize(750, 500);
    statusBar()->show();
    statusBar()->showMessage(trUtf8("I'm absolutely useful."));
    // setWindowIcon(QIcon("logoDD.png")); // have to find working dir again
}
