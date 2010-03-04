#ifndef DDCLIENT_GUI_ADD_DIALOG_H
#define DDCLIENT_GUI_ADD_DIALOG_H

#include <QDialog>
#include <QtGui/QLineEdit>
#include <QTextDocument>
#include <QtGui/QCheckBox>
#include <QComboBox>


class add_dialog : public QDialog{
    Q_OBJECT

    public:
        /** Constructor
        *   @param parent MainWindow, which calls the dialog
        */
        add_dialog(QWidget *parent);

    private:
        QLineEdit *url_single;
        QLineEdit *title_single;
        QComboBox *package_single;
        QComboBox *package_many;
        QTextDocument *add_many;

    private slots:
        void ok();
};

#endif // DDCLIENT_GUI_ADD_DIALOG_H
