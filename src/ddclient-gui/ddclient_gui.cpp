#include "ddclient_gui.h"
#include "ddclient_gui_update_thread.h"
#include "ddclient_gui_connect_dialog.h"
#include "ddclient_gui_add_dialog.h"

#include <sstream>
#include <fstream>
#include <iomanip>

#include <QtGui/QStatusBar>
#include <QtGui/QMenuBar>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QLabel>
#include <QToolBar>
#include <QStringList>
#include <QStandardItem>
#include <QtGui/QContextMenuEvent>
#include <QModelIndex>
//#include <QtGui> // this is only for testing if includes are the problem

using namespace std;


ddclient_gui::ddclient_gui(QString config_dir) : QMainWindow(NULL), config_dir(config_dir) {
    setWindowTitle("DownloadDaemon Client GUI");
    this->resize(750, 500);
    setWindowIcon(QIcon("img/logoDD.png"));

    lang.set_working_dir("lang/");
    dclient = new downloadc();

    statusBar()->show();
    status_connection = new QLabel(tsl("Not connected"));
    statusBar()->insertPermanentWidget(0, status_connection);

    add_bars();
    add_list_components();

    // connect if logindata was saved
    string file_name = config_dir.toStdString() + "save.dat";
    ifstream ifs(file_name.c_str(), fstream::in | fstream::binary); // open file

    if(ifs.good()){ // file successfully opened
        login_data last_data =  { "", 0, "", ""};
        ifs.read((char *) &last_data, sizeof(login_data));

        if(last_data.lang[0] != '\0') // older versions of save.dat won't have lang, so we have to check
            set_language(last_data.lang); // set program language

        try{
            dclient->connect(last_data.host, last_data.port, last_data.pass, true);
            update_status(last_data.host);

        }catch(client_exception &e){
            if(e.get_id() == 2){ // daemon doesn't allow encryption

                QMessageBox box(QMessageBox::Question, tsl("Auto Connection: No Encryption Supported"), tsl("Encrypted authentication not supported by server.") + ("\n")
                                +tsl("Do you want to try unsecure plain-text authentication?"), QMessageBox::Yes|QMessageBox::No);

                box.setModal(true);
                int del = box.exec();

                if(del == QMessageBox::YesRole){ // connect again
                    try{
                        dclient->connect(last_data.host, last_data.port, last_data.pass, false);
                    }catch(client_exception &e){}
                }
            } // we don't have error message here because it's an auto fuction
        }
    }

    connect(this, SIGNAL(do_reload()), this, SLOT(on_reload()));
    
    update_thread *thread = new update_thread(this);
    thread->start();
}


ddclient_gui::~ddclient_gui(){
    mx.lock();
    delete dclient;
    mx.unlock();
}


void ddclient_gui::update_status(QString server){
    if(!check_connection()){
        return;
    }
    
    string answer;
    mx.lock();
    try{
        answer = dclient->get_var("downloading_active");

    }catch(client_exception &e){}
    mx.unlock();

    // removing both icons/deactivating both menuentrys, even when maybe only one is shown
    activate_action->setEnabled(false);
    deactivate_action->setEnabled(false);
    downloading_menu->removeAction(activate_action);
    downloading_menu->removeAction(deactivate_action);
    
    if(answer == "1"){ // downloading active
        downloading_menu->addAction(deactivate_action);
        deactivate_action->setEnabled(true);
    }else if(answer =="0"){ // downloading not active
        downloading_menu->addAction(activate_action);
        activate_action->setEnabled(true);
    }else{
        // should never be reached
    }

    if(server != QString("")) // sometimes the function is called to update the toolbar, not the status text => server is empty
        status_connection->setText(tsl("Connected to") + " " + server);
}


QString ddclient_gui::tsl(string text, ...){
    string translated = lang[text];

    size_t n;
    int i = 1;
    va_list vl;
    va_start(vl, text);
    string search("p1");

    while((n = translated.find(search)) != string::npos){
	stringstream id;
	id << va_arg(vl, char *);
	translated.replace(n-1, 3, id.str());

	i++;
	if(i>9) // maximal 9 additional Parameters
	    break;

	stringstream sb;
	sb << "p" << i;
	search = sb.str();
    }

    return trUtf8(translated.c_str());
}


QMutex *ddclient_gui::get_mutex(){
    return &mx;
}


void ddclient_gui::set_language(std::string lang_to_set){
    lang.set_language(lang_to_set);

    update_bars();
    update_list_components();
}


downloadc *ddclient_gui::get_connection(){
    return dclient;
}


void ddclient_gui::get_content(){
    mx.lock();
    new_content.clear();

    try{
        new_content = dclient->get_list();

    }catch(client_exception &e){}

    // send event to reload list
    mx.unlock();
    emit do_reload();
}


bool ddclient_gui::check_connection(bool tell_user, string individual_message){
    mx.lock();

    try{
        dclient->check_connection();

    }catch(client_exception &e){
        if(e.get_id() == 10){ //connection lost

            if(tell_user)
                QMessageBox::information(this, tsl("No Connection to Server"), tsl(individual_message));
            mx.unlock();
            return false;
        }
    }
    mx.unlock();
    return true;
}


void ddclient_gui::add_bars(){
    // menubar
    file_menu = menuBar()->addMenu("&" + tsl("File"));
    help_menu = menuBar()->addMenu("&" + tsl("Help"));

    connect_action = new QAction(QIcon("img/1_connect.png"), "&" + tsl("Connect"), this);
    connect_action->setShortcut(QString("Alt+C"));
    connect_action->setStatusTip(tsl("Connect to a DownloadDaemon Server"));
    file_menu->addAction(connect_action);

    configure_action = new QAction(QIcon("img/8_configure.png"), tsl("Configure"), this);
    configure_action->setShortcut(QString("Alt+P"));
    configure_action->setStatusTip(tsl("Configure DownloadDaemon Server"));
    file_menu->addAction(configure_action);

    activate_action = new QAction(QIcon("img/9_activate.png"), tsl("Activate Downloading"), this);
    activate_action->setShortcut(QString("F2"));
    activate_action->setStatusTip(tsl("Activate Downloading"));
    file_menu->addAction(activate_action);
    activate_action->setEnabled(false);

    deactivate_action = new QAction(QIcon("img/9_deactivate.png"), tsl("Deactivate Downloading"), this);
    deactivate_action->setShortcut(QString("F3"));
    deactivate_action->setStatusTip(tsl("Deactivate Downloading"));
    file_menu->addAction(deactivate_action);
    deactivate_action->setEnabled(false);
    file_menu->addSeparator();

    activate_download_action = new QAction(QIcon("img/5_start.png"), tsl("Activate Download"), this);
    activate_download_action->setShortcut(QString("Alt+A"));
    activate_download_action->setStatusTip(tsl("Activate the selected Download"));
    file_menu->addAction(activate_download_action);

    deactivate_download_action = new QAction(QIcon("img/4_stop.png"), tsl("Deactivate Download"), this);
    deactivate_download_action->setShortcut(QString("Alt+D"));
    deactivate_download_action->setStatusTip(tsl("Deactivate the selected Download"));
    file_menu->addAction(deactivate_download_action);
    file_menu->addSeparator();

    add_action = new QAction(QIcon("img/2_add.png"), tsl("Add Download"), this);
    add_action->setShortcut(QString("Alt+I"));
    add_action->setStatusTip(tsl("Add Download"));
    file_menu->addAction(add_action);

    delete_action = new QAction(QIcon("img/3_delete.png"), tsl("Delete Download"), this);
    delete_action->setShortcut(QString("DEL"));
    delete_action->setStatusTip(tsl("Delete Download"));
    file_menu->addAction(delete_action);

    delete_finished_action = new QAction(QIcon("img/10_delete_finished.png"), tsl("Delete finished Downloads"), this);
    delete_finished_action->setShortcut(QString("Ctrl+DEL"));
    delete_finished_action->setStatusTip(tsl("Delete finished Downloads"));
    file_menu->addAction(delete_finished_action);
    file_menu->addSeparator();

    select_action = new QAction(QIcon("img/14_select_all.png"), tsl("Select all"), this);
    select_action->setShortcut(QString("Ctrl+A"));
    select_action->setStatusTip(tsl("Select all"));
    file_menu->addAction(select_action);

    copy_action = new QAction(QIcon("img/11_copy_url.png"), tsl("Copy URL"), this);
    copy_action->setShortcut(QString("Ctrl+C"));
    copy_action->setStatusTip(tsl("Copy URL"));
    file_menu->addAction(copy_action);
    file_menu->addSeparator();

    quit_action = new QAction(QIcon("img/12_quit.png"), "&" + tsl("Quit"), this);
    quit_action->setShortcut(QString("Alt+F4"));
    quit_action->setStatusTip(tsl("Quit"));
    file_menu->addAction(quit_action);

    about_action = new QAction(QIcon("img/13_about.png"), "&" + tsl("About"), this);
    about_action->setShortcut(QString("F1"));
    about_action->setStatusTip(tsl("About"));
    help_menu->addAction(about_action);

    // toolbar
    up_action = new QAction(QIcon("img/6_up.png"), "&" + tsl("Increase Priority"), this);
    up_action->setStatusTip(tsl("Increase Priority of the selected Download"));

    down_action = new QAction(QIcon("img/7_down.png"), "&" + tsl("Decrease Priority"), this);
    down_action->setStatusTip(tsl("Decrease Priority of the selected Download"));

    QToolBar *connect_menu = addToolBar(tsl("Connect"));
    connect_menu->addAction(connect_action);

    QToolBar *download_menu = addToolBar(tsl("Download"));
    download_menu->addAction(add_action);
    download_menu->addAction(delete_action);
    download_menu->addAction(delete_finished_action);
    download_menu->addSeparator();
    download_menu->addAction(activate_download_action);
    download_menu->addAction(deactivate_download_action);
    download_menu->addSeparator();
    download_menu->addAction(up_action);
    download_menu->addAction(down_action);

    QToolBar *configure_menu = addToolBar(tsl("Configure"));
    configure_menu->addAction(configure_action);

    downloading_menu = addToolBar(tsl("Downloading"));
    downloading_menu->addAction(activate_action);

    connect(connect_action, SIGNAL(triggered()), this, SLOT(on_connect()));
    connect(configure_action, SIGNAL(triggered()), this, SLOT(on_configure()));
    connect(activate_action, SIGNAL(triggered()), this, SLOT(on_downloading_activate()));
    connect(deactivate_action, SIGNAL(triggered()), this, SLOT(on_downloading_deactivate()));
    connect(activate_download_action, SIGNAL(triggered()), this, SLOT(on_activate()));
    connect(deactivate_download_action, SIGNAL(triggered()), this, SLOT(on_deactivate()));
    connect(add_action, SIGNAL(triggered()), this, SLOT(on_add()));
    connect(delete_action, SIGNAL(triggered()), this, SLOT(on_delete()));
    connect(delete_finished_action, SIGNAL(triggered()), this, SLOT(on_delete_finished()));
    connect(select_action, SIGNAL(triggered()), this, SLOT(on_select()));
    connect(copy_action, SIGNAL(triggered()), this, SLOT(on_copy()));
    connect(quit_action, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(about_action, SIGNAL(triggered()), this, SLOT(on_about()));
    connect(up_action, SIGNAL(triggered()), this, SLOT(on_priority_up()));
    connect(down_action, SIGNAL(triggered()), this, SLOT(on_priority_down()));
}


void ddclient_gui::update_bars(){
    file_menu->setTitle("&" + tsl("File"));
    help_menu->setTitle("&" + tsl("Help"));

    connect_action->setText("&" + tsl("Connect"));
    connect_action->setStatusTip(tsl("Connect to a DownloadDaemon Server"));

    configure_action->setText(tsl("Configure"));
    configure_action->setStatusTip(tsl("Configure DownloadDaemon Server"));

    activate_action->setText(tsl("Activate Downloading"));
    activate_action->setStatusTip(tsl("Activate Downloading"));

    deactivate_action->setText(tsl("Deactivate Downloading"));
    deactivate_action->setStatusTip(tsl("Deactivate Downloading"));

    activate_download_action->setText(tsl("Activate Download"));
    activate_download_action->setStatusTip(tsl("Activate the selected Download"));

    deactivate_download_action->setText(tsl("Deactivate Download"));
    deactivate_download_action->setStatusTip(tsl("Deactivate the selected Download"));

    add_action->setText(tsl("Add Download"));
    add_action->setStatusTip(tsl("Add Download"));

    delete_action->setText(tsl("Delete Download"));
    delete_action->setStatusTip(tsl("Delete Download"));

    delete_finished_action->setText(tsl("Delete finished Downloads"));
    delete_finished_action->setStatusTip(tsl("Delete finished Downloads"));

    select_action->setText(tsl("Select all"));
    select_action->setStatusTip(tsl("Select all"));

    copy_action->setText(tsl("Copy URL"));
    copy_action->setStatusTip(tsl("Copy URL"));

    quit_action->setText("&" + tsl("Quit"));
    quit_action->setStatusTip(tsl("Quit"));

    about_action->setText("&" + tsl("About"));
    about_action->setStatusTip(tsl("About"));

    up_action->setText("&" + tsl("Increase Priority"));
    up_action->setStatusTip(tsl("Increase Priority of the selected Download"));

    down_action->setText("&" + tsl("Decrease Priority"));
    down_action->setStatusTip(tsl("Decrease Priority of the selected Download"));
}


void ddclient_gui::add_list_components(){
    // create treeview which will show all the data later
    list_model = new QStandardItemModel();
    QStringList column_labels;
    column_labels << tsl("ID") << tsl("Title") << tsl("URL") << tsl("Time left") << tsl("Status");

    list = new QTreeView();
    list_model->setHorizontalHeaderLabels(column_labels);
    list->setModel(list_model);

    selection_model = new QItemSelectionModel(list_model);
    list->setSelectionModel(selection_model);
    list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list->setSelectionBehavior(QAbstractItemView::SelectRows);

    double width = list->width();

    list->setColumnWidth(0, 100); // fixed sizes
    list->setColumnWidth(3, 100);
    width -= 100;
    list->setColumnWidth(1, 0.25*width);
    list->setColumnWidth(2, 0.3*width);
    list->setColumnWidth(4, 0.45*width);

    setCentralWidget(list);
}


void ddclient_gui::update_list_components(){
    QStringList column_labels;
    column_labels << tsl("ID") << tsl("Title") << tsl("URL") << tsl("Time left") << tsl("Status");
    list_model->setHorizontalHeaderLabels(column_labels);
}


void ddclient_gui::cut_time(string &time_left){
    long time_span = atol(time_left.c_str());
    int hours = 0, mins = 0, secs = 0;
    stringstream stream_buffer;

    secs = time_span % 60;
    if(time_span >= 60) // cut time_span down to minutes
        time_span /= 60;
    else { // we don't have minutes
        stream_buffer << secs << " s";
        time_left = stream_buffer.str();
        return;
    }

    mins = time_span % 60;
    if(time_span >= 60) // cut time_span down to hours
        time_span /= 60;
    else { // we don't have hours
        stream_buffer << mins << ":";
        if(secs < 10)
            stream_buffer << "0";
        stream_buffer << secs << "m";
        time_left = stream_buffer.str();
        return;
    }

    hours = time_span;
    stream_buffer << hours << "h, ";
    if(mins < 10)
        stream_buffer << "0";
    stream_buffer << mins << ":";
    if(secs < 10)
        stream_buffer << "0";
    stream_buffer << secs << "m";

    time_left = stream_buffer.str();
    return;
}

string ddclient_gui::build_status(string &status_text, string &time_left, download &dl){
    string color;
    color = "white";

    if(dl.status == "DOWNLOAD_RUNNING"){
        color = "green";

        if(dl.wait > 0 && dl.error == "PLUGIN_SUCCESS"){ // waiting time > 0
            status_text = lang["Download running. Waiting."];
            stringstream time;
            time << dl.wait;
            time_left =  time.str();
            cut_time(time_left);

        }else if(dl.wait > 0 && dl.error != "PLUGIN_SUCCESS"){
            color = "red";

            status_text = lang["Error"] + ": " + lang[dl.error] + " " + lang["Retrying soon."];
            stringstream time;
            time << dl.wait;
            time_left =  time.str();
            cut_time(time_left);

        }else{ // no waiting time
            stringstream stream_buffer, time_buffer;
            stream_buffer << lang["Running"];

            if(dl.speed != 0 && dl.speed != -1) // download speed known
                stream_buffer << "@" << setprecision(1) << fixed << (float)dl.speed / 1024 << " kb/s";

            stream_buffer << ": ";

            if(dl.size == 0 || dl.size == 1){ // download size unknown
                stream_buffer << "0.00% - ";
                time_left = "";

                if(dl.downloaded == 0 || dl.downloaded == 1) // nothing downloaded yet
                    stream_buffer << "0.00 MB/ 0.00 MB";
                else // something downloaded
                    stream_buffer << setprecision(1) << fixed << (float)dl.downloaded / 1048576 << " MB/ 0.00 MB";

            }else{ // download size known
                if(dl.downloaded == 0 || dl.downloaded == 1){ // nothing downloaded yet
                    stream_buffer << "0.00% - 0.00 MB/ " << fixed << (float)dl.size / 1048576 << " MB";

                    if(dl.speed != 0 && dl.speed != -1){ // download speed known => calc time left
                        time_buffer << (int)(dl.size / dl.speed);
                        time_left = time_buffer.str();
                        cut_time(time_left);
                    }else
                        time_left = "";

                }else{ // download size known and something downloaded
                    stream_buffer << setprecision(1) << fixed << (float)dl.downloaded / (float)dl.size * 100 << "% - ";
                    stream_buffer << setprecision(1) << fixed << (float)dl.downloaded / 1048576 << " MB/ ";
                    stream_buffer << setprecision(1) << fixed << (float)dl.size / 1048576 << " MB";

                    if(dl.speed != 0 && dl.speed != -1){ // download speed known => calc time left
                        time_buffer << (int)((dl.size - dl.downloaded) / dl.speed);
                        time_left = time_buffer.str();
                        cut_time(time_left);
                    }else
                        time_left = "";
                }
            }
            status_text = stream_buffer.str();
        }

    }else if(dl.status == "DOWNLOAD_INACTIVE"){
        if(dl.error == "PLUGIN_SUCCESS"){
            color = "yellow";

            status_text = lang["Download Inactive."];
            time_left = "";

        }else{ // error occured
            color = "red";

            status_text = lang["Inactive. Error"] + ": " + lang[dl.error];
            time_left = "";
        }

    }else if(dl.status == "DOWNLOAD_PENDING"){
        time_left = "";

        if(dl.error == "PLUGIN_SUCCESS"){
            status_text = lang["Download Pending."];

        }else{ //error occured
            color = "red";

            status_text = lang["Error"] + ": " + lang[dl.error];
        }

    }else if(dl.status == "DOWNLOAD_WAITING"){
        color = "red";

        status_text = lang["Have to wait."];
        stringstream time;
        time << dl.wait;
        time_left =  time.str();
        cut_time(time_left);

    }else if(dl.status == "DOWNLOAD_FINISHED"){
        color = "star";

        status_text = lang["Download Finished."];
        time_left = "";

    }else if(dl.status == "DOWNLOAD_RECONNECTING") {
        color = "yellow";

        status_text = lang["Reconnecting..."];
        time_left = "";
    }else{ // default, column 4 has unknown input
        status_text = lang["Status not detected."];
        time_left = "";
    }

    return color;
}


void ddclient_gui::deselect_lines(){
    list->clearSelection();
}


void ddclient_gui::get_selected_lines(){
    selected_lines.clear();

    // find the selected indizes and save them into vector selected_lines
    QModelIndexList selection_list = selection_model->selectedRows();

    // find row, package (yes/no), parent_row
    for(int i=0; i<selection_list.size(); ++i){
        selected_info info;
        info.row = selection_list.at(i).row();

        if(selection_list.at(i).parent() == QModelIndex::QModelIndex()){ // no parent: we have a package
            info.package = true;
            info.parent_row = -1;
        }else{
            info.package = false;
            info.parent_row = selection_list.at(i).parent().row();
        }
    selected_lines.push_back(info);
    }
}


vector<view_info> ddclient_gui::get_current_view(){
    get_selected_lines();
    vector<view_info> info;

    vector<package>::iterator pit = content.begin();
    vector<selected_info>::iterator sit = selected_lines.begin();
    vector<download>::iterator dit;
    vector<view_info>::iterator vit;
    int line = 0;

    // loop all packages to see which are expanded
    for(; pit != content.end(); ++pit){
        view_info curr_info;
        curr_info.package = true;
        curr_info.id = pit->id;
        curr_info.package_id = -1;

        curr_info.expanded = list->isExpanded(list_model->index(line, 0, QModelIndex()));
        curr_info.selected = false;
        info.push_back(curr_info);

        // loop all downloads to save information for later
        for(dit = pit->dls.begin(); dit != pit->dls.end(); ++dit){
            view_info curr_d_info;
            curr_d_info.package = false;
            curr_d_info.id = dit->id;
            curr_d_info.package_id = pit->id;
            curr_d_info.expanded = false;
            curr_d_info.selected = false;
            info.push_back(curr_d_info);
        }
        line++;
    }

    // loop all selected lines to save that too
    for(; sit != selected_lines.end(); ++sit){
        if(sit->package){ // package
            unsigned int row = sit->row;
            if(content.size() < row)
                break; // shouldn't happen => someone deleted content!

             // loop info to find the package with the right id
            int id = content.at(row).id;

            for(vit = info.begin(); vit != info.end(); ++vit){ // find package with that in info
                if((vit->id == id) && (vit->package)){
                    vit->selected = true;
                    break;
                }
            }

        }else{ // download
            unsigned int row = sit->row;
            unsigned int parent_row = sit->parent_row;
            if(content.size() < parent_row)
                break; // shouldn't happen => someone deleted content!
            if(content.at(sit->parent_row).dls.size() < row)
                break;

            // loop info to find the download with the right id
            int id = (content.at(parent_row)).dls.at(row).id; // found real id!

            for(vit = info.begin(); vit != info.end(); ++vit){ // find download with that in info
                if((vit->id == id) && !(vit->package)){
                    vit->selected = true;
                    break;
                }
            }
        }
    }

    return info;
}

// slots
void ddclient_gui::on_about(){
    QMessageBox::information(this, "Test", "on_about");
}


void ddclient_gui::on_select(){
    if(!check_connection(false))
        return;

    list->expandAll();
    list->selectAll();
}


void ddclient_gui::on_connect(){
    connect_dialog dialog(this, config_dir);
    dialog.setModal(true);
    dialog.exec();

    get_content();
}


void ddclient_gui::on_add(){
    if(!check_connection(true, "Please connect before adding Downloads."))
        return;

    add_dialog dialog(this);
    dialog.setModal(true);
    dialog.exec();

    get_content();
}


void ddclient_gui::on_delete(){
    if(!check_connection(true, "Please connect before deleting Downloads."))
        return;

    mx.lock();
    get_selected_lines();

    vector<selected_info>::iterator it;
    int id;

    if(selected_lines.empty()){
        QMessageBox::warning(this, tsl("Error"), tsl("At least one Row should be selected."));
        mx.unlock();
        return;
    }

    // make sure user wants to delete downloads
    QMessageBox box(QMessageBox::Question, tsl("Delete Downloads"), tsl("Do you really want to delete\nthe selected Download(s)?"),
                    QMessageBox::Yes|QMessageBox::No, this);
    box.setModal(true);
    int del = box.exec();

    if(del != QMessageBox::Yes){ // user clicked no to delete
        mx.unlock();
        return;
    }


    int dialog_answer = QMessageBox::No; // possible answers are YesToAll, Yes, No, NoToAll
    int package_id = -1;
    vector<int> package_deletes; // saves how many dowloads of each package have been deleted to see if the package is empty afterwards
    unsigned int package_count = 0;

    for(it = selected_lines.begin(); it < selected_lines.end(); it++){

        if(it->package){ // we have a package! => only save number and count downloads deleted

            if(package_id != -1){ // so we don't delete the default value
                package_deletes.push_back(package_count);
                package_count = 0;
            }

            package_id = content.at(it->row).id;
            package_deletes.push_back(package_id);

        }else{ // we have a real download
            id = content.at(it->parent_row).dls.at(it->row).id;
            package_count++;
        }

        try{
            if(!(it->package)) // we have a real download, not a package
                dclient->delete_download(id, dont_know);

        }catch(client_exception &e){
            if(e.get_id() == 7){

                if((dialog_answer != QMessageBox::YesToAll) && (dialog_answer != QMessageBox::NoToAll)){ // file exists and user didn't choose YesToAll or NoToAll before
                    stringstream s;
                    s << id;
                    QMessageBox file_box(QMessageBox::Question, tsl("Delete File"), tsl("Do you want to delete the downloaded File for Download %p1?", s.str().c_str()),
                                         QMessageBox::YesToAll|QMessageBox::Yes|QMessageBox::No|QMessageBox::NoToAll, this);
                    file_box.setModal(true);
                    dialog_answer = file_box.exec();
                }

                if((dialog_answer == QMessageBox::YesToAll) || (dialog_answer == QMessageBox::Yes)){
                    try{
                        dclient->delete_download(id, del_file);

                    }catch(client_exception &e){
                        stringstream s;
                        s << id;
                        QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting File of Download %p1.", s.str().c_str()));
                        package_count--; // download delete failed
                    }

                }else{ // don't delete file
                    try{
                        dclient->delete_download(id, dont_delete);

                    }catch(client_exception &e){
                            QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting Download(s)."));
                            package_count--; // download delete failed
                    }
                }
            }else{ // some error occured
                QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting Download(s)."));
                package_count--; // download delete failed
            }
        }
    }
    package_deletes.push_back(package_count);

    // see if we can delete the packages (count == dls.size)
    vector<int>::iterator pit;
    vector<package>::iterator contentpackage_it;
    unsigned int size;

    for(pit = package_deletes.begin(); pit < package_deletes.end(); pit++){
        package_id = *pit;
        pit++;
        package_count = *pit;

        for(contentpackage_it = content.begin(); contentpackage_it < content.end(); contentpackage_it++){
            if(contentpackage_it->id == package_id){
                size = contentpackage_it->dls.size();
                break;
            }
        }

        if(size <= package_count){ // we deleted all downloads of that package
            try{
                dclient->delete_package(package_id);
            }catch(client_exception &e){}
        }
    }

    mx.unlock();
    get_content();
}


void ddclient_gui::on_delete_finished(){
    if(!check_connection(true, "Please connect before deleting Downloads."))
        return;

    vector<int>::iterator it;
    vector<int> finished_ids;
    vector<package>::iterator content_it;
    vector<download>::iterator download_it;
    int id;
    int package_count = 0;


    mx.lock();
    vector<int> package_deletes; // prepare structure to save how many downloads of which package will be deleted
    for(unsigned int i = 0; i < content.size(); i++)
        package_deletes.push_back(0);

    // delete all empty packages
    for(content_it = content.begin(); content_it < content.end(); content_it++){
        if(content_it->dls.size() == 0){
            try{
                dclient->delete_package(content_it->id);
            }catch(client_exception &e){}
        }
    }

    // find all finished downloads
    for(content_it = content.begin(); content_it < content.end(); content_it++){
        for(download_it = content_it->dls.begin(); download_it < content_it->dls.end(); download_it++){
            if(download_it->status == "DOWNLOAD_FINISHED"){
                finished_ids.push_back(download_it->id);
                package_deletes[package_count]++;
            }
        }
        package_count++;
    }


    if(!finished_ids.empty()){
        // make sure user wants to delete downloads
        QMessageBox box(QMessageBox::Question, tsl("Delete Downloads"), tsl("Do you really want to delete\nall finished Download(s)?"),
                        QMessageBox::Yes|QMessageBox::No, this);
        box.setModal(true);
        int del = box.exec();

        if(del != QMessageBox::Yes){ // user clicked no to delete
            mx.unlock();
            return;
        }


        int dialog_answer = QMessageBox::No; // possible answers are YesToAll, Yes, No, NoToAll

        for(it = finished_ids.begin(); it < finished_ids.end(); it++){
                id = *it;

            try{
                dclient->delete_download(id, dont_know);

            }catch(client_exception &e){
                if(e.get_id() == 7){

                    if((dialog_answer != QMessageBox::YesToAll) && (dialog_answer != QMessageBox::NoToAll)){ // file exists and user didn't choose YesToAll or NoToAll before
                    stringstream s;
                    s << id;
                    QMessageBox file_box(QMessageBox::Question, tsl("Delete File"), tsl("Do you want to delete the downloaded File for Download %p1?", s.str().c_str()),
                                         QMessageBox::YesToAll|QMessageBox::Yes|QMessageBox::No|QMessageBox::NoToAll, this);
                    file_box.setModal(true);
                    dialog_answer = file_box.exec();
                }

                    if((dialog_answer == QMessageBox::YesToAll) || (dialog_answer == QMessageBox::Yes)){
                        try{
                            dclient->delete_download(id, del_file);

                        }catch(client_exception &e){
                            stringstream s;
                            s << id;
                            QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting File of Download %p1.", s.str().c_str()));
                        }

                    }else{ // don't delete file
                        try{
                            dclient->delete_download(id, dont_delete);

                        }catch(client_exception &e){
                                QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting Download(s)."));
                        }

                    }
                }else{ // some error occured
                    QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting Download(s)."));
                }
            }
        }

    }
    deselect_lines();

    // delete all empty packages
    int i = 0;
    for(it = package_deletes.begin(); it < package_deletes.end(); it++){
        unsigned int package_size = *it;

        if(content.at(i).dls.size() <= package_size){ // if we deleted every download inside a package
            try{
                dclient->delete_package(content.at(i).id);
            }catch(client_exception &e){}
        }
        i++;
    }

    mx.unlock();
    get_content();
}


void ddclient_gui::on_delete_file(){
    if(!check_connection(true, "Please connect before deleting Files."))
        return;

    mx.lock();
    get_selected_lines();

    vector<selected_info>::iterator it;
    string answer;
    int id;

    if(selected_lines.empty()){
        QMessageBox::warning(this, tsl("Error"), tsl("At least one Row should be selected."));
        mx.unlock();
        return;
    }

    // make sure user wants to delete files
    QMessageBox box(QMessageBox::Question, tsl("Delete Files"), tsl("Do you really want to delete\nthe selected File(s)?"),
                    QMessageBox::Yes|QMessageBox::No, this);
    box.setModal(true);
    int del = box.exec();

    if(del != QMessageBox::Yes){ // user clicked no to delete
        mx.unlock();
        return;
    }

    for(it = selected_lines.begin(); it < selected_lines.end(); it++){

        if(it->package) // we have a package
            continue; // next one

        else // we have a real download
            id = content.at(it->parent_row).dls.at(it->row).id;

        try{
            dclient->delete_file(id);
        }catch(client_exception &e){
            if(e.get_id() == 19){ // file error
                stringstream s;
                s << id;
                QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting File of Download %p1.", s.str().c_str()));
            }
        }
    }

    mx.unlock();
    deselect_lines();
    get_content();
}


void ddclient_gui::on_activate(){
    if(!check_connection(true, "Please connect before activating Downloads."))
        return;

    QMessageBox::information(this, "Test", "on_activate");
    get_selected_lines();
                                                                                               // TODO: not finished!
    get_content();
}


void ddclient_gui::on_deactivate(){
    if(!check_connection(true, "Please connect before deactivating Downloads."))
        return;

    QMessageBox::information(this, "Test", "on_deactivate");
    get_selected_lines();
                                                                                               // TODO: not finished!
    get_content();
}


void ddclient_gui::on_priority_up(){
    if(!check_connection(true, "Please connect before increasing Priority."))
        return;

    QMessageBox::information(this, "Test", "on_priority_up");
    get_selected_lines();
                                                                                               // TODO: not finished!
    get_content();
}


void ddclient_gui::on_priority_down(){
    if(!check_connection(true, "Please connect before decreasing Priority."))
        return;

    QMessageBox::information(this, "Test", "on_priority_down");
    get_selected_lines();
                                                                                               // TODO: not finished!
    get_content();
}


void ddclient_gui::on_configure(){
    if(!check_connection(true, "Please connect before configurating DownloadDaemon."))
        return;

    QMessageBox::information(this, "Test", "on_configure");
                                                                                               // TODO: not finished!
}


void ddclient_gui::on_downloading_activate(){
    if(!check_connection(true, "Please connect before you activate Downloading."))
        return;

    mx.lock();
    try{
        dclient->set_var("downloading_active", "1");
    }catch(client_exception &e){}
    mx.unlock();

    // update toolbar
    activate_action->setEnabled(false);
    deactivate_action->setEnabled(true);
    downloading_menu->removeAction(activate_action);
    downloading_menu->removeAction(deactivate_action); // just to be save
    downloading_menu->addAction(deactivate_action);
}


void ddclient_gui::on_downloading_deactivate(){
    if(!check_connection(true, "Please connect before you deactivate Downloading."))
        return;

    mx.lock();
    try{
        dclient->set_var("downloading_active", "0");
    }catch(client_exception &e){}
    mx.unlock();

    // update toolbar
    activate_action->setEnabled(true);
    deactivate_action->setEnabled(false);
    downloading_menu->removeAction(activate_action);
    downloading_menu->removeAction(deactivate_action); // just to be save
    downloading_menu->addAction(activate_action);
}


void ddclient_gui::on_copy(){
    if(!check_connection(true, "Please connect before copying URLs."))
        return;

    QMessageBox::information(this, "Test", "on_copy");
    get_selected_lines();
                                                                                               // TODO: not finished!
    get_content();
}


void ddclient_gui::on_reload(){
    if(!check_connection()){
        status_connection->setText(tsl("Not connected"));
        return;
    }

    mx.lock();
    vector<view_info> info = get_current_view();

    bool new_list = false;
    if(content.size() < 1)
        new_list = true;
    content.clear();
    content = new_content;

    list_model->clear();

    QStringList column_labels;
    column_labels << tsl("ID") << tsl("Title") << tsl("URL") << tsl("Time left") << tsl("Status");
    list_model->setHorizontalHeaderLabels(column_labels);

    double width = list->width();

    list->setColumnWidth(0, 100); // fixed sizes
    list->setColumnWidth(3, 100);
    width -= 250;
    list->setColumnWidth(1, 0.25*width);
    list->setColumnWidth(2, 0.3*width);
    list->setColumnWidth(4, 0.45*width);

    vector<package>::iterator pit = content.begin();
    vector<download>::iterator dit;
    vector<view_info>::iterator vit;

    QStandardItem *pkg;
    QStandardItem *dl;
    int line = 0;
    string color, status_text, time_left;
    bool expanded;

    for(; pit != content.end(); pit++){ // loop all packages
        pkg = new QStandardItem(QIcon("img/package.png"), QString("%1").arg(pit->id));
        pkg->setEditable(false);
        list_model->setItem(line, 0, pkg);

        QModelIndex index = list_model->index(line, 0, QModelIndex()); // downloads need the parent index

        // recreate expansion and selection
        expanded = false;
        for(vit = info.begin(); vit != info.end(); ++vit){
            if((vit->id == pit->id) && (vit->package)){
                if(vit->expanded) // package is expanded
                    expanded = true;

                if(vit->selected){ // package is selected
                    for(int i=0; i<5; i++)
                        selection_model->select(list_model->index(line, i, QModelIndex()), QItemSelectionModel::Select);
                }
                break;
            }
        }

        int dl_line = 0;
        for(dit = pit->dls.begin(); dit != pit->dls.end(); dit++){ // loop all downloads of that package
            color = build_status(status_text, time_left, *dit);

            dl = new QStandardItem(QIcon("img/bullet_black.png"), QString("%1").arg(dit->id));
            dl->setEditable(false);
            pkg->setChild(dl_line, 0, dl);

            dl = new QStandardItem(QString(dit->title.c_str()));
            dl->setEditable(false);
            pkg->setChild(dl_line, 1, dl);

            dl = new QStandardItem(QString(dit->url.c_str()));
            dl->setEditable(false);
            pkg->setChild(dl_line, 2, dl);

            dl = new QStandardItem(QString(time_left.c_str()));
            dl->setEditable(false);
            pkg->setChild(dl_line, 3, dl);

            string colorstring = "img/bullet_" + color + ".png";
            dl = new QStandardItem(QIcon(colorstring.c_str()), QString(status_text.c_str()));
            dl->setEditable(false);
            pkg->setChild(dl_line, 4, dl);

            // recreate selection if existed
            for(vit = info.begin(); vit != info.end(); ++vit){
                if((vit->id == dit->id) && !(vit->package)){
                    if(vit->selected){ // download is selected
                        for(int i=0; i<5; i++)
                            selection_model->select(index.child(dl_line, i), QItemSelectionModel::Select);

                        break;
                    }
                }
            }

            dl_line++;
        }

        pkg = new QStandardItem(QString(pit->name.c_str()));
        pkg->setEditable(false);
        list_model->setItem(line, 1, pkg);

        for(int i=2; i<5; i++){
            pkg = new QStandardItem(QString(""));
            pkg->setEditable(false);
            list_model->setItem(line, i, pkg);
        }

        if(expanded)
            list->expand(index);

        line++;
    }

    mx.unlock();
    if(new_list)
        list->expandAll();
}


void ddclient_gui::contextMenuEvent(QContextMenuEvent *event){
    QMenu menu(this);

    QAction* delete_file_action = new QAction(QIcon("img/15_delete_file.png"), tsl("Delete File"), this);
    delete_file_action->setShortcut(QString("Ctrl+F"));
    delete_file_action->setStatusTip(tsl("Delete File"));
    menu.addAction(delete_file_action);
    menu.addSeparator();

    menu.addAction(activate_download_action);
    menu.addAction(deactivate_download_action);
    menu.addSeparator();
    menu.addAction(delete_action);
    menu.addAction(delete_file_action);
    menu.addSeparator();
    menu.addAction(select_action);
    menu.addAction(copy_action);

    connect(activate_download_action, SIGNAL(triggered()), this, SLOT(on_activate()));
    connect(deactivate_download_action, SIGNAL(triggered()), this, SLOT(on_deactivate()));
    connect(delete_action, SIGNAL(triggered()), this, SLOT(on_delete()));
    connect(delete_file_action, SIGNAL(triggered()), this, SLOT(on_delete_file()));
    connect(select_action, SIGNAL(triggered()), this, SLOT(on_select()));
    connect(copy_action, SIGNAL(triggered()), this, SLOT(on_copy()));

    menu.exec(event->globalPos());
}

void ddclient_gui::resizeEvent(QResizeEvent* event){
    event = event;
    double width = list->width();

    width -= 250;
    list->setColumnWidth(1, 0.25*width);
    list->setColumnWidth(2, 0.3*width);
    list->setColumnWidth(4, 0.45*width);
}
