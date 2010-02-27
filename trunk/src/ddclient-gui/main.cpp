#include "ddclient_gui.h"
#include <QtGui/QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ddclient_gui c;
    c.show();
    return a.exec();
}

