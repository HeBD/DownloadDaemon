#ifndef DDCLIENT_GUI_CONNECT_DIALOG_H
#define DDCLIENT_GUI_CONNECT_DIALOG_H

#include <QDialog>
#include <QtGui/QLineEdit>
#include <QtGui/QCheckBox>
#include <QComboBox>


class connect_dialog : public QDialog{
    Q_OBJECT

    public:
        /** Defaultconstructor */
        connect_dialog(QWidget *parent, QString config_dir);

    private:
        QLineEdit *host;
        QLineEdit *port;
        QLineEdit *password;
        QComboBox *language;
        QCheckBox *save_data;
        QString config_dir;

    private slots:
        void ok();

};

#endif // DDCLIENT_GUI_CONNECT_DIALOG_H
