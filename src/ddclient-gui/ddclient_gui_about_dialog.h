#ifndef DDCLIENT_GUI_ABOUT_DIALOG_H
#define DDCLIENT_GUI_ABOUT_DIALOG_H

#include <QDialog>


class about_dialog : public QDialog{
    public:
        /** Constructor
        *   @param parent MainWindow, which calls the dialog
        *   @param build buildinformation
        */
        about_dialog(QWidget *parent, QString build);
};

#endif // DDCLIENT_GUI_ABOUT_DIALOG_H
