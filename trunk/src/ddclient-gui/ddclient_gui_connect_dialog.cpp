#include "ddclient_gui_connect_dialog.h"
#include "ddclient_gui.h"
#include <string>
#include <fstream>

#include <QtGui/QGroupBox>
#include <QtGui/QFormLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QDialogButtonBox>
#include <QtGui/QMessageBox>

using namespace std;

connect_dialog::connect_dialog(QWidget *parent, QString config_dir) : QDialog(parent), config_dir(config_dir){
    ddclient_gui *p = (ddclient_gui *) parent;

    setWindowTitle(p->tsl("Connect to Host"));

    // read saved data if it exists
    string file_name = string(config_dir.toStdString()) + "save.dat";
    ifstream ifs(file_name.c_str(), fstream::in | fstream::binary); // open file

    login_data last_data =  { "", 0, "", ""};
    if((ifs.rdstate() & ifstream::failbit) == 0){ // file successfully opened

        ifs.read((char *) &last_data,sizeof(login_data));
    }
    ifs.close();

    if(strcmp(last_data.host, "") == 0){ // data wasn't read from file => default
        snprintf(last_data.host, 256, "127.0.0.1");
        last_data.port = 56789;
        last_data.pass[0] = '\0';
        snprintf(last_data.lang, 128, "English");
    }



    QGroupBox *connect_box = new QGroupBox(p->tsl("Data"));
    QFormLayout *form_layout = new QFormLayout();
    QVBoxLayout *layout = new QVBoxLayout();
    QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect_box->setLayout(form_layout);
    layout->addWidget(new QLabel(p->tsl("Please insert Host Data")));
    layout->addWidget(connect_box);
    layout->addWidget(button_box);
    this->setLayout(layout);

    host = new QLineEdit(last_data.host);
    port = new QLineEdit(QString("%1").arg(last_data.port));
    port->setMaxLength(5);
    port->setFixedWidth(50);
    password = new QLineEdit();
    password->setEchoMode(QLineEdit::Password);
    language = new QComboBox();
    language->addItem("English");
    language->addItem("Deutsch");
    save_data = new QCheckBox();
    save_data->setChecked(true);

    if(strcmp(last_data.lang, "Deutsch") == 0)
        language->setCurrentIndex(1);
    else // English is default
        language->setCurrentIndex(0);

    form_layout->addRow(new QLabel(p->tsl("IP/URL")), host);
    form_layout->addRow(new QLabel(p->tsl("Port")), port);
    form_layout->addRow(new QLabel(p->tsl("Password")), password);
    form_layout->addRow(new QLabel(p->tsl("Language")), language);
    form_layout->addRow(new QLabel(p->tsl("Save Data")), save_data);

    connect(button_box->button(QDialogButtonBox::Ok), SIGNAL(clicked()),this, SLOT(ok()));
    connect(button_box->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),this, SLOT(reject()));
}

void connect_dialog::ok(){
    string host = this->host->text().toStdString();
    int port = this->port->text().toInt();
    string password = this->password->text().toStdString();
    string language = this->language->currentText().toStdString();

    bool error_occured = false;
    ddclient_gui *p = (ddclient_gui *) parent();
    p->set_language(language);
    //boost::mutex *mx = p->get_mutex();
    downloadc *dclient = p->get_connection();

    //mx->lock();
    try{
        dclient->connect(host, port, password, true);

    }catch(client_exception &e){
        if(e.get_id() == 2){ // daemon doesn't allow encryption

            QMessageBox box(QMessageBox::Question, p->tsl("No Encryption"), p->tsl("Encrypted authentication not supported by server.") + ("\n")
                           + p->tsl("Do you want to try unsecure plain-text authentication?"), QMessageBox::Yes|QMessageBox::No);

            box.setModal(true);
            int del = box.exec();

            if(del == QMessageBox::YesRole){ // connect again
                try{
                    dclient->connect(host, port, password, false);
                }catch(client_exception &e){

                    // standard connection error handling
                    if(e.get_id() == 1){ // wrong host/port
                        QMessageBox::critical(this,  p->tsl("Connection failed"), p->tsl("Connection failed (wrong IP/URL or Port).")  + ("\n")
                                              + p->tsl("Please try again."));
                        error_occured = true;

                    }else if(e.get_id() == 3){ // wrong password
                        QMessageBox::critical(this,  p->tsl("Authentication Error"), p->tsl("Wrong Password, Authentication failed."));
                        error_occured = true;

                    }else if(e.get_id() == 4){ // wrong password
                        QMessageBox::critical(this,  p->tsl("Authentication Error"), p->tsl("Unknown Error while Authentication."));
                        error_occured = true;

                    }else{ // we have some other connection error
                        QMessageBox::critical(this,  p->tsl("Authentication Error"), p->tsl("Unknown Error while Authentication."));
                        error_occured = true;
                    }

                }
            }

        // standard connection error handling
        }else if(e.get_id() == 1){ // wrong host/port
            QMessageBox::critical(this,  p->tsl("Connection failed"), p->tsl("Connection failed (wrong IP/URL or Port).")  + ("\n")
                                  + p->tsl("Please try again."));
            error_occured = true;

        }else if(e.get_id() == 3){ // wrong password
            QMessageBox::critical(this,  p->tsl("Authentication Error"), p->tsl("Wrong Password, Authentication failed."));
            error_occured = true;

        }else if(e.get_id() == 4){ // wrong password
            QMessageBox::critical(this,  p->tsl("Authentication Error"), p->tsl("Unknown Error while Authentication."));
            error_occured = true;

        }else{ // we have some other connection error
            QMessageBox::critical(this,  p->tsl("Authentication Error"), p->tsl("Unknown Error while Authentication."));
            error_occured = true;
        }
    }
    //mx->unlock();

    // save data if no error occured => connection established
    if(!error_occured){

        // updated statusbar of parent
        p->update_status(this->host->text());

        // write login_data to a file
        if(save_data->isChecked()){ // user clicked checkbox to save data

            string file_name = string(config_dir.toStdString()) + "save.dat";
            ofstream ofs(file_name.c_str(), fstream::out | fstream::binary | fstream::trunc); // open file

            login_data last_data;
            snprintf(last_data.host, 256, "%s", host.c_str());
            last_data.port = port;
            snprintf(last_data.pass, 256, "%s", password.c_str());
            snprintf(last_data.lang, 128, "%s", language.c_str());

            if(ofs.good()){ // file successfully opened

                ofs.write((char *) &last_data, sizeof(login_data));
            }
            ofs.close();
        }

        emit done(0);
    }
}

