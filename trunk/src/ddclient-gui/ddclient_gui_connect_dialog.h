#ifndef DDCLIENT_GUI_CONNECT_DIALOG_H
#define DDCLIENT_GUI_CONNECT_DIALOG_H

#include <QDialog>
#include <QtGui/QLineEdit>
#include <QtGui/QCheckBox>
#include <QComboBox>
#include <cfgfile/cfgfile.h>
#include "ddclient_gui.h"


class connect_dialog : public QDialog{
    Q_OBJECT

    public:
        /** Constructor
        *   @param parent MainWindow, which calls the dialog
        *   @param config_dir Configuration Directory
        */
        connect_dialog(QWidget *parent, QString config_dir);

    private:
	QComboBox *host;
        QLineEdit *port;
        QLineEdit *password;
        QComboBox *language;
        QCheckBox *save_data;
        QString config_dir;
	cfgfile file;
	login_data data;

    private slots:
	void host_selected();
        void ok();

};

#endif // DDCLIENT_GUI_CONNECT_DIALOG_H
