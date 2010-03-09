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

    many_layout->addWidget(new QLabel(p->tsl("Separate URL and Title like this: http://something.aa/bb|a fancy Title")));
    many_layout->addLayout(many_package_layout);
    many_layout->addWidget(add_many_edit);

    connect(button_box->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(ok()));
    connect(button_box->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
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

    // exchange every | inside the title with a blank
    size_t title_find;
    title_find = title.find("|");

    while(title_find != string::npos){
        title.at(title_find) = ' ';
        title_find = title.find("|");
    }

    // find out if we have an existing or new package;
    int package_single_id = -1;
    int package_many_id = -1;
    vector<package_info>::iterator it = packages.begin();

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


    bool error_occured = false;
    int error = 0;
    vector<string> splitted_line;
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

        // send a single download
        try{
            if(package_many_id == -1) // create a new package
                package_many_id = dclient->add_package(package_many);

            dclient->add_download(package_many_id, url, title);
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
    mx->unlock();

    emit done(0);
}

