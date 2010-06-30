/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient_gui_add_dialog.h"
#include "ddclient_gui.h"
#include <sstream>

#include <QtGui/QGroupBox>
#include <QtGui/QFormLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QDialogButtonBox>
#include <QtGui/QMessageBox>
#include <QtGui/QTextEdit>


using namespace std;

add_dialog::add_dialog(QWidget *parent) : QDialog(parent){
    ddclient_gui *p = (ddclient_gui *) parent;

    setWindowTitle(p->tsl("Add Downloads"));

    QGroupBox *single_box = new QGroupBox(p->tsl("single Download"));
    QGroupBox *many_box = new QGroupBox(p->tsl("many Downloads"));
    QFormLayout *single_layout = new QFormLayout();
    QFormLayout *many_package_layout = new QFormLayout();
    QVBoxLayout *layout = new QVBoxLayout();
    QVBoxLayout *many_layout = new QVBoxLayout();
    QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    single_box->setLayout(single_layout);
    many_box->setLayout(many_layout);

    layout->addWidget(new QLabel(p->tsl("Add single Download (no Title necessary)")));
    layout->addWidget(single_box);
    layout->addWidget(new QLabel(""));
    layout->addWidget(new QLabel(p->tsl("Add many Downloads (one per Line, no Title necessary)")));
    layout->addWidget(many_box);
    layout->addWidget(button_box);
    this->setLayout(layout);

    // single download
    url_single = new QLineEdit();
    title_single = new QLineEdit();
    title_single->setFixedWidth(200);
    package_single = new QComboBox();
    package_single->setFixedWidth(200);
    package_single->setEditable(true);

    // many downloads
    QTextEdit *add_many_edit = new QTextEdit();
    add_many_edit->setFixedHeight(150);
    add_many = new QTextDocument(add_many_edit);
    add_many_edit->setDocument(add_many);
    package_many = new QComboBox();
    package_many->setFixedWidth(200);
    package_many->setEditable(true);
    separate_packages = new QCheckBox(p->tsl("Separate into different Packages"));

    QMutex *mx = p->get_mutex();
    downloadc *dclient = p->get_connection();
    vector<package> pkgs;
    vector<package>::iterator it;

    mx->lock();
    try{
        pkgs = dclient->get_packages();
    }catch(client_exception &e){}
    mx->unlock();

    for(it = pkgs.begin(); it != pkgs.end(); it++){
        if(it->name != string("")){
            package_single->addItem(QString("%1").arg(it->id) + ": " + it->name.c_str());
            package_many->addItem(QString("%1").arg(it->id) + ": " + it->name.c_str());
        }else{
            package_single->addItem(QString("%1").arg(it->id));
            package_many->addItem(QString("%1").arg(it->id));
        }
        package_info info = {it->id, it->name};
        packages.push_back(info);
    }

    button_box->button(QDialogButtonBox::Ok)->setDefault(true);
    button_box->button(QDialogButtonBox::Ok)->setFocus(Qt::OtherFocusReason);

    single_layout->addRow(new QLabel(p->tsl("Package")), package_single);
    single_layout->addRow(new QLabel(p->tsl("Title")), title_single);
    single_layout->addRow(new QLabel(p->tsl("URL")), url_single);

    many_package_layout->addRow(new QLabel(p->tsl("Package")), package_many);
    many_package_layout->addRow(new QLabel(""), separate_packages);

    many_layout->addWidget(new QLabel(p->tsl("Separate URL and Title like this: http://something.aa/bb|a fancy Title")));
    many_layout->addLayout(many_package_layout);
    many_layout->addWidget(add_many_edit);

    connect(button_box->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(ok()));
    connect(button_box->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
    connect(separate_packages, SIGNAL(stateChanged(int)), this, SLOT(separate_packages_toggled()));
}


void add_dialog::find_parts(vector<new_download> &all_dls){
    ddclient_gui *p = (ddclient_gui *) parent();
    downloadc *dclient = p->get_connection();
    int package_id;
    bool error_occured = false;
    int error = 0;

    // prepare imaginary file names
    vector<new_download>::iterator it = all_dls.begin();
    for(; it != all_dls.end(); ++it){
        it->file_name = it->url;

        // cut away some things
        size_t n;

        n = it->file_name.find_last_of("//");
        if(n != (it->file_name.length())-1) // cut everything till the last / if there is something after it, eg: http://cut.me/bla => bla
            it->file_name = it->file_name.substr(n+1);

        else{ // there is a / at the end, delete it and do it again
            it->file_name.erase(n, 1);

            n = it->file_name.find_last_of("//");
            if(n != (it->file_name.length())-1) // cut everything till the last / if there is something after it, eg: http://cut.me/bla => bla
                it->file_name = it->file_name.substr(n+1);

        }

        n = it->file_name.find_last_of("?");
        if(n != (it->file_name.length())-1) // cut everything from the last ?, eg: test?var1=5 => test
            it->file_name = it->file_name.substr(0, n);

        string cut_me = ".html";
        while((n = it->file_name.find(cut_me)) != std::string::npos) // cut .html
                it->file_name.replace(n, cut_me.length(), "");

        cut_me = ".htm";
        while((n = it->file_name.find(cut_me)) != std::string::npos) // cut .htm
                it->file_name.replace(n, cut_me.length(), "");

        for (size_t i = 0; i < it->file_name.size(); ++i){ // strip numbers
            if(isdigit(it->file_name[i])){
                it->file_name.erase(i, 1);
                --i; // otherwise the next letter will not be looked at
            }
        }

        for (size_t i = 0; i < it->file_name.size(); ++i) // make it case insensitive
            it->file_name[i] = tolower(it->file_name[i]);
    }


    // decide packages based on the file names
    vector<new_download>::iterator inner_it;

    for(it = all_dls.begin(); it != all_dls.end(); ++it){
        if(it->package == -1){ // found a download without a package

            // create a new package
            try{
                package_id = dclient->add_package();
            }catch(client_exception &e){ // there is no reason to continue if we failed here
                QMessageBox::warning(this,  p->tsl("Error"), p->tsl("Failed to create Package."));
                return;
            }

            it->package = package_id;

            // find all other downloads with the same file name
            for(inner_it = it+1; inner_it != all_dls.end(); ++inner_it){

                if(inner_it->package == -1){ // no package yet
                    if(inner_it->file_name == it->file_name)
                        inner_it->package = package_id;
                }
            }
        }
    }


    // finally send downloads
    for(it = all_dls.begin(); it != all_dls.end(); ++it){
        try{
            dclient->add_download(it->package, it->url, it->title);
        }catch(client_exception &e){
            error_occured = true;
            error = e.get_id();
        }

    }



    if(error_occured){
        if(error == 6)
            QMessageBox::warning(this,  p->tsl("Error"), p->tsl("Failed to create Package."));
        else if(error == 13)
            QMessageBox::warning(this,  p->tsl("Invalid URL"), p->tsl("At least one inserted URL was invalid."));
    }

}


void add_dialog::separate_packages_toggled(){
    if(separate_packages->isChecked())
        package_many->setEnabled(false);
    else
        package_many->setEnabled(true);
}


void add_dialog::ok(){
    ddclient_gui *p = (ddclient_gui *) parent();
    downloadc *dclient = p->get_connection();
    QMutex *mx = p->get_mutex();

    string title = title_single->text().toStdString();
    string url = url_single->text().toStdString();
    string many = add_many->toPlainText().toStdString();
    string package_single = this->package_single->currentText().toStdString();
    string package_many = this->package_many->currentText().toStdString();

    bool separate = separate_packages->isChecked();

    // exchange every | inside the title (of single download) with a blank
    size_t title_find;
    title_find = title.find("|");

    while(title_find != string::npos){
        title.at(title_find) = ' ';
        title_find = title.find("|");
    }

    // find out if we have an existing or new package
    int package_single_id = -1;
    int package_many_id = -1;
    vector<package_info>::iterator it = packages.begin();

    if(package_single.size() != 0){ // we don't have to check if there's no package name given
        for(; it != packages.end(); ++it){ // first single package
            stringstream s;
            s << it->id;
            if(package_single == s.str()){
                package_single_id = it->id;
                break;
            }

            s << ": " << it->name;
            if(package_single == s.str()){
                package_single_id = it->id;
                break;
            }
        }
    }

    if((package_many.size() != 0) && !separate){ // we don't have to check if there's no package name given and we don't have to separate into packages
        for(it = packages.begin(); it != packages.end(); ++it){ // now many package
            stringstream s;
            s << it->id;
            if(package_many == s.str()){
                package_many_id = it->id;
                break;
            }

            s << ": " << it->name;
            if(package_many == s.str()){
                package_many_id = it->id;
                break;
            }
        }
    }


    bool error_occured = false;
    int error = 0;
    string line;
    size_t lineend = 1, urlend;

    // add single download
    mx->lock();
    if(!url.empty()){
        try{
            if(package_single_id == -1) // create a new package
                package_single_id = dclient->add_package(package_single);

            dclient->add_download(package_single_id, url, title);
        }catch(client_exception &e){
            error_occured = true;
            error = e.get_id();
        }
    }

    // add many downloads
    vector<new_download> all_dls; // in case we have to separate into packages
    new_download dl = {"", "", "", -1};

    // parse lines
    while(many.length() > 0 && lineend != string::npos){
        lineend = many.find("\n"); // termination character for line

        if(lineend == string::npos){ // this is the last line (which ends without \n)
            line = many.substr(0, many.length());
            many = "";

        }else{ // there is still another line after this one
            line = many.substr(0, lineend);
            many = many.substr(lineend+1);
        }

        // parse url and title
        urlend = line.find("|");

        if(urlend != string::npos){ // we have a title
            url = line.substr(0, urlend);
            line = line.substr(urlend+1);

            urlend = line.find("|");
            while(urlend != string::npos){ // exchange every | with a blank (there can be some | inside the title too)
                line.at(urlend) = ' ';
                urlend = line.find("|");
            }
            title = line;

        }else{ // no title
            url = line;
            title = "";
        }

        if(separate){ // don't send it now, find fitting packages first
            dl.url = url;
            dl.title = title;
            all_dls.push_back(dl);

        }else{ // send a single download
            try{
                if(package_many_id == -1) // create a new package
                    package_many_id = dclient->add_package(package_many);

                dclient->add_download(package_many_id, url, title);
            }catch(client_exception &e){
                error_occured = true;
                error = e.get_id();
            }
        }
    }

    if(all_dls.size() > 0){
        find_parts(all_dls);
    }

    if(error_occured){
        if(error == 6)
            QMessageBox::warning(this,  p->tsl("Error"), p->tsl("Failed to create Package."));
        else if(error == 13)
            QMessageBox::warning(this,  p->tsl("Invalid URL"), p->tsl("At least one inserted URL was invalid."));
    }
    mx->unlock();

    emit done(0);
}

