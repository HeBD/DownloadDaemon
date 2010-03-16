/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_GUI_ADD_DIALOG_H
#define DDCLIENT_GUI_ADD_DIALOG_H

#include <QDialog>
#include <QtGui/QLineEdit>
#include <QTextDocument>
#include <QtGui/QCheckBox>
#include <QComboBox>

struct package_info{
    int id;
    std::string name;
};


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
        std::vector<package_info> packages;

    private slots:
        void ok();
};

#endif // DDCLIENT_GUI_ADD_DIALOG_H
