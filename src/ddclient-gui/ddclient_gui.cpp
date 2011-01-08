/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient_gui.h"
#include "ddclient_gui_update_thread.h"
#include "ddclient_gui_add_dialog.h"
#include "ddclient_gui_about_dialog.h"
#include "ddclient_gui_configure_dialog.h"
#include "ddclient_gui_connect_dialog.h"
#include "ddclient_gui_captcha_dialog.h"
#include "ddclient_gui_status_bar.h"

#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cfgfile/cfgfile.h>

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
#include <QtGui/QInputDialog>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
//#include <QtGui> // this is only for testing if includes are the problem

using namespace std;


ddclient_gui::ddclient_gui(QString config_dir) : QMainWindow(NULL), config_dir(config_dir), full_list_update(true), reload_list(false) {
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

	if(QSystemTrayIcon::isSystemTrayAvailable())
		add_tray_icon();

	// connect if logindata was saved
	string file_name = config_dir.toStdString() + "ddclient-gui.conf";
	cfgfile file;
	file.open_cfg_file(file_name, false);

	if(file){ // file successfully opened
	login_data data;
	data.selected = file.get_int_value("selected");
	stringstream s;
	s << data.selected;
	data.host.push_back(file.get_cfg_value("host" + s.str()));
	data.port.push_back(file.get_int_value("port" + s.str()));
	data.pass.push_back(file.get_cfg_value("pass" + s.str()));

	set_language(file.get_cfg_value("language")); // set program language
		status_connection->setText(tsl("Not connected"));

		try{
		dclient->connect(data.host[0], data.port[0], data.pass[0], true);
		update_status(data.host[0].c_str());

		}catch(client_exception &e){
			if(e.get_id() == 2){ // daemon doesn't allow encryption

				QMessageBox box(QMessageBox::Question, tsl("Auto Connection: No Encryption Supported"), tsl("Encrypted authentication not supported by server.") + ("\n")
								+tsl("Do you want to try unsecure plain-text authentication?"), QMessageBox::Yes|QMessageBox::No);

				box.setModal(true);
				int del = box.exec();

				if(del == QMessageBox::YesRole){ // connect again
					try{
			dclient->connect(data.host[0], data.port[0], data.pass[0], false);
					}catch(client_exception &e){}
				}
			} // we don't have an error message here because it's an auto function
		}
	}

	connect(this, SIGNAL(do_reload()), this, SLOT(on_reload()));

	int interval = file.get_int_value("update_interval");

	if(interval == 0)
		interval = 2;

	update_thread *thread = new update_thread(this, interval);
	thread->start();
	this->thread = thread;
}


ddclient_gui::~ddclient_gui(){
	mx.lock();
	dclient->set_term(true);
	delete dclient;
	((update_thread *)thread)->terminate_yourself();
	thread->wait();
	delete thread;
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
	configure_menu->removeAction(activate_action);
	configure_menu->removeAction(deactivate_action);

	if(answer == "1"){ // downloading active
		configure_menu->addAction(deactivate_action);
		deactivate_action->setEnabled(true);
	}else if(answer =="0"){ // downloading not active
		configure_menu->addAction(activate_action);
		activate_action->setEnabled(true);
	}else{
		// should never be reached
	}

	if(server != QString("")){ // sometimes the function is called to update the toolbar, not the status text => server is empty
		status_connection->setText(tsl("Connected to") + " " + server);
		this->server = server;
	}
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

	mx.lock();
	content.clear();
	full_list_update = true;
	mx.unlock();

	// send event to reload list
	get_content();
	emit do_reload();
}


void ddclient_gui::set_update_interval(int interval){
	((update_thread *) thread)->set_update_interval(interval);
}


downloadc *ddclient_gui::get_connection(){
	return dclient;
}


void ddclient_gui::get_content(bool update){
	vector<package> tmp_content;
	vector<update_content> tmp_updates;

	try{
		if(update && !reload_list){ // receive updates
			tmp_updates = dclient->get_updates();

			mx.lock();
			new_updates.clear();
			new_updates = tmp_updates;
			mx.unlock();

		}else{ // receive whole list
			full_list_update = true;
			reload_list = false;
			tmp_content = dclient->get_list();

			mx.lock();
			new_content.clear();
			new_content = tmp_content;
			mx.unlock();
		}

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
			status_connection->setText(tsl("Not connected"));

			if(tell_user && (last_error_message != error_connected)){
				QMessageBox::information(this, tsl("No Connection to Server"), tsl(individual_message));
				last_error_message = error_connected;

				if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
					this->show();
					this->hide();
				}
				emit do_reload();
			}
			mx.unlock();
			return false;
		}
	}
	mx.unlock();
	return true;
}


bool ddclient_gui::check_subscritpion(){
	mx.lock();

	try{
		dclient->add_subscription(SUBS_DOWNLOADS);
		dclient->add_subscription(SUBS_CONFIG);
		mx.unlock();
		return true;
	}catch(client_exception &e){
		mx.unlock();
		return false;
	}
}


void ddclient_gui::clear_last_error_message(){
	last_error_message = error_none;
}


int ddclient_gui::calc_package_progress(int package_row){
	int progress = 0, downloads = 0, finished = 0;

	try{
		downloads = content.at(package_row).dls.size();
		for(int i = 0; i < downloads; ++i){
			if(content.at(package_row).dls.at(i).status == "DOWNLOAD_FINISHED")
				finished++;
		}

		if(downloads != 0) // we don't want x/0
			progress = (finished * 100) / downloads;


	}catch(...){}

	return progress;
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

	download_menu = file_menu->addMenu(tsl("Download"));

	activate_download_action = new QAction(QIcon("img/5_start.png"), tsl("Activate Download"), this);
	activate_download_action->setShortcut(QString("Alt+A"));
	activate_download_action->setStatusTip(tsl("Activate the selected Download"));
	download_menu->addAction(activate_download_action);

	deactivate_download_action = new QAction(QIcon("img/4_stop.png"), tsl("Deactivate Download"), this);
	deactivate_download_action->setShortcut(QString("Alt+D"));
	deactivate_download_action->setStatusTip(tsl("Deactivate the selected Download"));
	download_menu->addAction(deactivate_download_action);
	download_menu->addSeparator();

	container_action = new QAction(QIcon("img/16_container.png"), tsl("Add Download Container"), this);
	container_action->setStatusTip(tsl("Add Download Container"));
	download_menu->addAction(container_action);

	add_action = new QAction(QIcon("img/2_add.png"), tsl("Add Download"), this);
	add_action->setShortcut(QString("Alt+I"));
	add_action->setStatusTip(tsl("Add Download"));
	download_menu->addAction(add_action);

	delete_action = new QAction(QIcon("img/3_delete.png"), tsl("Delete Download"), this);
	delete_action->setShortcut(QString("DEL"));
	delete_action->setStatusTip(tsl("Delete Download"));
	download_menu->addAction(delete_action);

	delete_finished_action = new QAction(QIcon("img/10_delete_finished.png"), tsl("Delete finished Downloads"), this);
	delete_finished_action->setShortcut(QString("Ctrl+DEL"));
	delete_finished_action->setStatusTip(tsl("Delete finished Downloads"));
	download_menu->addAction(delete_finished_action);

	captcha_action = new QAction(QIcon("img/bullet_captcha.png"), "&" + tsl("Enter Captcha"), this);
	captcha_action->setStatusTip(tsl("Enter Captcha"));
	download_menu->addSeparator();
	download_menu->addAction(captcha_action);

	file_menu->addSeparator();
	select_action = new QAction(QIcon("img/14_select_all.png"), tsl("Select all"), this);
	select_action->setShortcut(QString("Ctrl+A"));
	select_action->setStatusTip(tsl("Select all"));
	file_menu->addAction(select_action);

	copy_action = new QAction(QIcon("img/11_copy_url.png"), tsl("Copy URL"), this);
	copy_action->setShortcut(QString("Ctrl+C"));
	copy_action->setStatusTip(tsl("Copy URL"));
	file_menu->addAction(copy_action);

	paste_action = new QAction(QIcon("img/11_copy_url.png"), tsl("Paste URL"), this);
	paste_action->setShortcut(QString("Ctrl+V"));
	paste_action->setStatusTip(tsl("Paste URL"));
	file_menu->addAction(paste_action);
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
	download_menu->addSeparator();
	download_menu->addAction(captcha_action);

	configure_menu = addToolBar(tsl("Configure"));
	configure_menu->addAction(configure_action);
	configure_menu->addAction(activate_action);

	QToolBar *donate_bar = addToolBar(tsl("Flattr"));
	QAction *flattr_action = new QAction(QIcon("img/flattr.png"), "Flattr", this);
	donate_bar->addAction(flattr_action);
	QAction *donate_action = new QAction(QIcon("img/coins.png"), "Project-Support", this);
	donate_bar->addAction(donate_action);

	connect(connect_action, SIGNAL(triggered()), this, SLOT(on_connect()));
	connect(configure_action, SIGNAL(triggered()), this, SLOT(on_configure()));
	connect(activate_action, SIGNAL(triggered()), this, SLOT(on_downloading_activate()));
	connect(deactivate_action, SIGNAL(triggered()), this, SLOT(on_downloading_deactivate()));
	connect(activate_download_action, SIGNAL(triggered()), this, SLOT(on_activate()));
	connect(deactivate_download_action, SIGNAL(triggered()), this, SLOT(on_deactivate()));
	connect(container_action, SIGNAL(triggered()), this, SLOT(on_load_container()));

	connect(add_action, SIGNAL(triggered()), this, SLOT(on_add()));
	connect(delete_action, SIGNAL(triggered()), this, SLOT(on_delete()));
	connect(delete_finished_action, SIGNAL(triggered()), this, SLOT(on_delete_finished()));
	connect(select_action, SIGNAL(triggered()), this, SLOT(on_select()));
	connect(copy_action, SIGNAL(triggered()), this, SLOT(on_copy()));
	connect(paste_action, SIGNAL(triggered()), this, SLOT(on_paste()));
	connect(quit_action, SIGNAL(triggered()), qApp, SLOT(quit()));
	connect(about_action, SIGNAL(triggered()), this, SLOT(on_about()));
	connect(up_action, SIGNAL(triggered()), this, SLOT(on_priority_up()));
	connect(down_action, SIGNAL(triggered()), this, SLOT(on_priority_down()));
	connect(captcha_action, SIGNAL(triggered()), this, SLOT(on_enter_captcha()));

	connect(flattr_action, SIGNAL(triggered()), this, SLOT(donate_flattr()));
	connect(donate_action, SIGNAL(triggered()), this, SLOT(donate_sf()));
}


void ddclient_gui::update_bars(){
	file_menu->setTitle("&" + tsl("File"));
	help_menu->setTitle("&" + tsl("Help"));
	download_menu->setTitle( tsl("Download"));

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

	container_action->setText(tsl("Add Download Container"));
	container_action->setStatusTip(tsl("Add Download Container"));

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

	paste_action->setText(tsl("Paste URL"));
	paste_action->setStatusTip(tsl("Paste URL"));

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
	connect(selection_model, SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(on_reload()));


	status_bar *status_bar_delegate = new status_bar(this);
	list->setItemDelegateForColumn(4, status_bar_delegate);
	list->setAnimated(true);

	setCentralWidget(list);
}


void ddclient_gui::update_list_components(){
	QStringList column_labels;
	column_labels << tsl("ID") << tsl("Title") << tsl("URL") << tsl("Time left") << tsl("Status");
	list_model->setHorizontalHeaderLabels(column_labels);
}


void ddclient_gui::add_tray_icon(){
	tray_menu = new QMenu(this);
	tray_menu->addAction(connect_action);
	tray_menu->addAction(configure_action);
	tray_menu->addSeparator();
	tray_menu->addAction(activate_action);
	tray_menu->addAction(deactivate_action);
	tray_menu->addSeparator();
	tray_menu->addAction(paste_action);
	tray_menu->addSeparator();
	tray_menu->addAction(about_action);
	tray_menu->addAction(quit_action);

	tray_icon = new QSystemTrayIcon(QIcon("img/logoDD.png"), this);
	tray_icon->setContextMenu(tray_menu);
	tray_icon->show();

	connect(tray_icon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(on_activate_tray_icon(QSystemTrayIcon::ActivationReason)));
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
	status_text = time_left = "";

	if(dl.status == "DOWNLOAD_RUNNING"){
		color = "green";

		if(dl.error == "Captcha") {
			color = "captcha";

			status_text = lang["Enter Captcha"] + ".";
			time_left = "";

		}else if(dl.wait > 0 && dl.error == "PLUGIN_SUCCESS"){ // waiting time > 0
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

			if(dl.speed != 0 && dl.speed != -1){ // download speed known
				stream_buffer << "@" << setprecision(1) << fixed << (double)dl.speed / 1024 << " kb/s";

				download_speed += (double)dl.speed / 1024;
			}

			stream_buffer << ": ";

			if(dl.size == 0 || dl.size == 1){ // download size unknown
				stream_buffer << "0.00% - ";
				time_left = "";

				if(dl.downloaded == 0 || dl.downloaded == 1) // nothing downloaded yet
					stream_buffer << "0.00 MB/ 0.00 MB";
				else // something downloaded
					stream_buffer << setprecision(1) << fixed << (double)dl.downloaded / 1048576 << " MB/ 0.00 MB";

			}else{ // download size known
				if(dl.downloaded == 0 || dl.downloaded == 1){ // nothing downloaded yet
					stream_buffer << "0.00% - 0.00 MB/ " << fixed << (double)dl.size / 1048576 << " MB";

					not_downloaded_yet += (double)dl.size / 1048576;

					if(dl.speed != 0 && dl.speed != -1){ // download speed known => calc time left
						time_buffer << (int)(dl.size / dl.speed);
						time_left = time_buffer.str();
						cut_time(time_left);
					}else
						time_left = "";

				}else{ // download size known and something downloaded
					stream_buffer << setprecision(1) << fixed << (double)dl.downloaded / (double)dl.size * 100 << "% - ";
					stream_buffer << setprecision(1) << fixed << (double)dl.downloaded / 1048576 << " MB/ ";
					stream_buffer << setprecision(1) << fixed << (double)dl.size / 1048576 << " MB";

					not_downloaded_yet += ((double)dl.size / 1048576) - (double)dl.downloaded / 1048576;

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

			if(dl.size == dl.downloaded && dl.size != 0) // download is finished but got inactivated!
				status_text = lang["Download Inactive and Finished."];
			else
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

			if(dl.size > 0){
				stringstream text;
				text << lang["Download Pending."] << " " << lang["Size"] << ": ";
				text << setprecision(1) << fixed << (double)dl.size / 1048576 << " MB";
				status_text = text.str();

				not_downloaded_yet += (double)dl.size / 1048576;

			}else{
				status_text = lang["Download Pending."];
			}

		}else{ //error occured
			color = "red";

			status_text = lang["Error"] + ": " + lang[dl.error];
		}

	}else if(dl.status == "DOWNLOAD_WAITING"){
		color = "yellow";

		status_text = lang["Have to wait."];
		stringstream time;
		time << dl.wait;
		time_left =  time.str();
		cut_time(time_left);

		if(dl.size > 0)
			not_downloaded_yet += ((double)dl.size / 1048576);

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


bool ddclient_gui::check_selected(){
	if(selected_lines.empty()){
		//if(last_error_message != error_selected) // user doesn't get told atm
		//	QMessageBox::warning(this, tsl("Error"), tsl("At least one Row should be selected."));

		last_error_message = error_selected;
		return false;
	}

	return true;
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

		if(selection_list.at(i).parent() == QModelIndex()){ // no parent: we have a package
			info.package = true;
			info.parent_row = -1;
		}else{
			info.package = false;
			info.parent_row = selection_list.at(i).parent().row();
		}
	selected_lines.push_back(info);
	}

	sort(selected_lines.begin(), selected_lines.end(), ddclient_gui::sort_selected_info);
}


bool ddclient_gui::sort_selected_info(selected_info i1, selected_info i2){
	if(i1.package && i2.package) // we have two packages
		return (i1.row < i2.row);

	if(i1.package){ // i1 is a package, i2 is a download
		if(i1.row == i2.parent_row) // see if they are from the same package
			return true; // package is smaller

		return (i1.row < i2.parent_row);

	}

	if(i2.package){ // i1 is a download, i2 is a package
		if(i2.row == i1.parent_row) // see if they are from the same package
			return false; // package is smaller

		return (i1.parent_row < i2.row);
	}

	// we have two downloads
	return (i1.parent_row < i2.parent_row);
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


void ddclient_gui::update_packages(){
	vector<package>::iterator pkg_it;
	vector<download>::iterator dl_it;
	vector<update_content>::iterator up_it = new_updates.begin();
	package pkg;
	download dl;
	QStandardItem *pkg_gui;
	QStandardItem *dl_gui;
	int line_nr, dl_line;
	bool exists;
	string colorstring, color, status_text, time_left;

	for(; up_it != new_updates.end(); ++up_it){
		exists = false;

		if((up_it->sub == SUBS_CONFIG) && (up_it->var_name == "downloading_active")){

			// update toolbar
			if(up_it->value == "1"){
				activate_action->setEnabled(false);
				deactivate_action->setEnabled(true);
				configure_menu->removeAction(activate_action);
				configure_menu->removeAction(deactivate_action); // just to be save
				configure_menu->addAction(deactivate_action);

			}else{
				activate_action->setEnabled(true);
				deactivate_action->setEnabled(false);
				configure_menu->removeAction(activate_action);
				configure_menu->removeAction(deactivate_action); // just to be save
				configure_menu->addAction(activate_action);
			}

			continue;
		}


		if(up_it->sub != SUBS_DOWNLOADS) // we just need SUBS_DOWNLOADS information
			continue;

		if(up_it->package){ // dealing with a package update

			if(up_it->reason == R_NEW){ // new package

				for(pkg_it = content.begin(); pkg_it != content.end(); ++pkg_it){
					if(up_it->id == pkg_it->id) // found right package
						exists = true;
				}

				if(exists) // got a new update even though we already have the package in the list
					continue;

				pkg.id = up_it->id;
				pkg.password = up_it->password;
				pkg.name = up_it->name;

				content.push_back(pkg);
				line_nr = list_model->rowCount();

				pkg_gui = new QStandardItem(QIcon("img/package.png"), QString("%1").arg(pkg.id));
				pkg_gui->setEditable(false);
				list_model->setItem(line_nr, 0, pkg_gui);

				pkg_gui = new QStandardItem(QString(pkg.name.c_str()));
				pkg_gui->setEditable(false);

				if(pkg.password != "")
					pkg_gui->setIcon(QIcon("img/key.png"));
				list_model->setItem(line_nr, 1, pkg_gui);

				for(int i=2; i<5; i++){
					pkg_gui = new QStandardItem(QString(""));
					pkg_gui->setEditable(false);
					list_model->setItem(line_nr, i, pkg_gui);
				}

				continue;
			}else if(up_it->reason == R_MOVEUP){
				// instead of messing with the list we just get the whole new list
				// user interactions cause a new list reload anyway (and if another connected user used move it really gets confusing)
				// => reload is the easiest way to keep the list up to date
				reload_list = true;
				continue;
			}else if(up_it->reason == R_MOVEDOWN){
				// same as MOVEUP
				reload_list = true;
				continue;
			}

			line_nr = 0;
			for(pkg_it = content.begin(); pkg_it != content.end(); ++pkg_it){

				if(up_it->id == pkg_it->id){ // found right package

					if(up_it->reason == R_UPDATE){
						pkg_it->name = up_it->name;
						pkg_it->password = up_it->password;

						pkg_gui = list_model->item(line_nr, 1);
						pkg_gui->setText(QString(up_it->name.c_str()));

						if(up_it->password != "")
							pkg_gui->setIcon(QIcon("img/key.png"));
						else
							pkg_gui->setIcon(QIcon());

					}else if(up_it->reason == R_DELETE){
						list_model->removeRow(line_nr);
						content.erase(pkg_it);
						break;
					}
				}

				++line_nr;

			}

		}else{ // dealing with a download

			if(up_it->reason == R_MOVEUP){
				// instead of messing with the list we just get the whole new list
				// user interactions cause a new list reload anyway (and if another connected user used move it really gets confusing)
				// => reload is the easiest way to keep the list up to date
				reload_list = true;
				continue;
			}else if(up_it->reason == R_MOVEDOWN){
				// same as MOVEUP
				reload_list = true;
				continue;
			}


			// find right package
			line_nr = 0;
			for(pkg_it = content.begin(); pkg_it != content.end(); ++pkg_it){
				if(up_it->pkg_id == pkg_it->id){ // found right package
					exists = true;
					break;
				}
				line_nr++;
			}

			if(!exists) // couldn't find right package
				continue;

			exists = false;

			dl_line = 0;
			for(dl_it = pkg_it->dls.begin(); dl_it != pkg_it->dls.end(); ++dl_it){
				if(up_it->id == dl_it->id){ // found right download
					exists = true;
					break;
				}
				dl_line++;
			}

			QModelIndex index = list_model->index(line_nr, 0, QModelIndex()); // downloads need the parent index
			pkg_gui = list_model->itemFromIndex(index);


			if(up_it->reason == R_NEW){ // new download

				if(exists) // download already exists
					continue;

				dl.id = up_it->id;
				dl.date = up_it->date;
				dl.title = up_it->title;
				dl.url = up_it->url;
				dl.status = up_it->status;
				dl.downloaded = up_it->downloaded;
				dl.size = up_it->size;
				dl.wait = up_it->wait;
				dl.error = up_it->error;
				dl.speed = up_it->speed;

				pkg_it->dls.push_back(dl);

				dl_line = pkg_it->dls.size() - 1;

				color = build_status(status_text, time_left, dl);

				dl_gui = new QStandardItem(QIcon("img/bullet_black.png"), QString("%1").arg(up_it->id));
				dl_gui->setEditable(false);
				pkg_gui->setChild(dl_line, 0, dl_gui);

				dl_gui = new QStandardItem(QString(up_it->title.c_str()));
				dl_gui->setEditable(false);
				pkg_gui->setChild(dl_line, 1, dl_gui);

				dl_gui = new QStandardItem(QString(up_it->url.c_str()));
				dl_gui->setEditable(false);
				pkg_gui->setChild(dl_line, 2, dl_gui);

				dl_gui = new QStandardItem(QString(time_left.c_str()));
				dl_gui->setEditable(false);
				pkg_gui->setChild(dl_line, 3, dl_gui);

				string colorstring = "img/bullet_" + color + ".png";
				dl_gui = new QStandardItem(QIcon(colorstring.c_str()), trUtf8(status_text.c_str()));
				dl_gui->setEditable(false);
				pkg_gui->setChild(dl_line, 4, dl_gui);

				list->expand(index);

				continue;


			}else if(up_it->reason == R_UPDATE){
				if(!exists) // couldn't find right download
					continue;

				if(dl_it->title != up_it->title){
					dl_it->title = up_it->title;

					dl_gui = pkg_gui->child(dl_line, 1);
					if(dl_gui != NULL)
						dl_gui->setText(QString(up_it->title.c_str()));
				}

				if(dl_it->url != up_it->url){
					dl_it->url = up_it->url;

					dl_gui = pkg_gui->child(dl_line, 2);
					if(dl_gui != NULL)
						dl_gui->setText(QString(up_it->url.c_str()));
				}

				if((dl_it->status != up_it->status) || (dl_it->downloaded != up_it->downloaded) || (dl_it->size != up_it->size) ||
				   (dl_it->wait != up_it->wait) || (dl_it->error != up_it->error) || (dl_it->speed != up_it->speed)){

					dl_it->status = up_it->status;
					dl_it->downloaded = up_it->downloaded;
					dl_it->size = up_it->size;
					dl_it->wait = up_it->wait;
					dl_it->error = up_it->error;
					dl_it->speed = up_it->speed;
					dl_it->title = up_it->title;

					color = build_status(status_text, time_left, *dl_it);

					dl_gui = pkg_gui->child(dl_line, 3);
					if(dl_gui != NULL)
						dl_gui->setText(QString(time_left.c_str()));

					string colorstring = "img/bullet_" + color + ".png";

					dl_gui = pkg_gui->child(dl_line, 4);
					if(dl_gui != NULL){
						dl_gui->setText(QString(trUtf8(status_text.c_str())));
						dl_gui->setIcon(QIcon(colorstring.c_str()));
					}

				}
			}else if(up_it->reason == R_DELETE){
				if(!exists) // couldn't find right download
					continue;

				pkg_gui->takeRow(dl_line);
				pkg_it->dls.erase(dl_it);
			}
		}
	}

	// calculate information for statusbar display
	vector<view_info>::iterator vit;
	vector<view_info> info = get_current_view();

	download_speed = 0;
	not_downloaded_yet = 0;
	selected_downloads_size = 0;
	selected_downloads_count = 0;

	for(pkg_it = content.begin(); pkg_it != content.end(); ++pkg_it){
		for(dl_it = pkg_it->dls.begin(); dl_it != pkg_it->dls.end(); ++dl_it){

			for(vit = info.begin(); vit != info.end(); ++vit){
				if(!(vit->package) && (vit->id == dl_it->id) && (vit->selected)){ // the download we have is selected

					if(dl_it->size != 0 && dl_it->size != 1){
						selected_downloads_size += (double)dl_it->size / 1048576;
						selected_downloads_count++;
					}
					break;
				}
			}


			if(dl_it->status == "DOWNLOAD_RUNNING" && dl_it->speed != 0 && dl_it->speed != -1)
				download_speed += (double)dl_it->speed / 1024;

			if(dl_it->size != 0 && dl_it->size != 1){

				if(dl_it->status == "DOWNLOAD_RUNNING" || dl_it->status == "DOWNLOAD_PENDING" || dl_it->status == "DOWNLOAD_WAITING"){

					if(dl_it->downloaded == 0 || dl_it->downloaded == 1)
						not_downloaded_yet += (double)dl_it->size / 1048576;
					else
						not_downloaded_yet += ((double)dl_it->size / 1048576) - (double)dl_it->downloaded / 1048576;
				}
			}
		}
	}
}


void ddclient_gui::compare_packages(){
	vector<package>::iterator old_it = content.begin();
	vector<package>::iterator new_it = new_content.begin();
	vector<download>::iterator dit;
	vector<view_info>::iterator vit;
	int line_nr = 0;
	bool expanded;
	string color, status_text, time_left;
	QStandardItem *pkg;
	QStandardItem *dl;

	vector<view_info> info = get_current_view();
	deselect_lines();
	list->setAnimated(false);

	// loop all packages
	while((old_it != content.end()) && (new_it != new_content.end())){

		// compare package items
		QModelIndex index = list_model->index(line_nr, 0, QModelIndex()); // downloads need the parent index
		expanded = false;

		if(old_it->id != new_it->id){
			pkg = list_model->item(line_nr, 0);
			pkg->setText(QString("%1").arg(new_it->id));
		}

		// recreate expansion and selection
		for(vit = info.begin(); vit != info.end(); ++vit){
			if((vit->id == new_it->id) && (vit->package)){
				if(vit->expanded) // package is expanded
					expanded = true;

				if(vit->selected){ // package is selected
					for(int i=0; i<5; i++)
						selection_model->select(list_model->index(line_nr, i, QModelIndex()), QItemSelectionModel::Select);
				}
				break;
			}
		}

		// compare downloads
		compare_downloads(index, new_it, old_it, info);

		if((old_it->name != new_it->name) || (old_it->password != new_it->password)){
			pkg = list_model->item(line_nr, 1);
			if(new_it->password != "")
				pkg->setIcon(QIcon("img/key.png"));
			else if(old_it->password  != "")
				pkg->setIcon(QIcon(""));
			pkg->setText(QString(new_it->name.c_str()));
		}

		if(expanded)
			list->expand(index);

		++old_it;
		++new_it;
		line_nr++;
	}

	if(old_it != content.end()){ // there are more old lines than new ones
		while(old_it != content.end()){

			// delete packages out of model
			list_model->removeRow(line_nr);
			++old_it;
		}

	}else if(new_it != new_content.end()){ // there are more new lines then old ones
		while(new_it != new_content.end()){
			// insert new lines

			pkg = new QStandardItem(QIcon("img/package.png"), QString("%1").arg(new_it->id));
			pkg->setEditable(false);
			list_model->setItem(line_nr, 0, pkg);

			QModelIndex index = list_model->index(line_nr, 0, QModelIndex()); // downloads need the parent index

			//insert downloads
			int dl_line = 0;
			for(dit = new_it->dls.begin(); dit != new_it->dls.end(); dit++){ // loop all downloads of that package
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
				dl = new QStandardItem(QIcon(colorstring.c_str()), trUtf8(status_text.c_str()));
				dl->setEditable(false);
				pkg->setChild(dl_line, 4, dl);

				dl_line++;
			}

			pkg = new QStandardItem(QString(new_it->name.c_str()));
			pkg->setEditable(false);
			if(new_it->password != "")
				pkg->setIcon(QIcon("img/key.png"));
			list_model->setItem(line_nr, 1, pkg);

			for(int i=2; i<5; i++){
				pkg = new QStandardItem(QString(""));
				pkg->setEditable(false);
				list_model->setItem(line_nr, i, pkg);
			}

			list->expand(index);

			line_nr++;
			++new_it;
		}
	}

	list->setAnimated(true);
}


void ddclient_gui::compare_downloads(QModelIndex &index, std::vector<package>::iterator &new_it, std::vector<package>::iterator &old_it, vector<view_info> &info){
	int dl_line = 0;
	vector<download>::iterator old_dit = old_it->dls.begin();
	vector<download>::iterator new_dit = new_it->dls.begin();
	vector<view_info>::iterator vit;
	QStandardItem *pkg;
	QStandardItem *dl;
	string color, status_text, time_left;

	pkg = list_model->itemFromIndex(index);

	// compare every single download of the package
	while((old_dit != old_it->dls.end()) && (new_dit != new_it->dls.end())){
		color = build_status(status_text, time_left, *new_dit);

		if(old_dit->id != new_dit->id){
			dl = pkg->child(dl_line, 0);
			dl->setText(QString("%1").arg(new_dit->id));
		}

		if(old_dit->title != new_dit->title){
			dl = pkg->child(dl_line, 1);
			dl->setText(QString(new_dit->title.c_str()));
		}

		if(old_dit->url != new_dit->url){
			dl = pkg->child(dl_line, 2);
			dl->setText(QString(new_dit->url.c_str()));
		}

		if((new_dit->status != old_dit->status) || (new_dit->downloaded != old_dit->downloaded) || (new_dit->size != old_dit->size) ||
		   (new_dit->wait != old_dit->wait) || (new_dit->error != old_dit->error) || (new_dit->speed != old_dit->speed)){

			dl = pkg->child(dl_line, 3);
			dl->setText(QString(time_left.c_str()));

			string colorstring = "img/bullet_" + color + ".png";

			dl = pkg->child(dl_line, 4);
			dl->setText(QString(trUtf8(status_text.c_str())));
			dl->setIcon(QIcon(colorstring.c_str()));

		}

		// recreate selection if existed
		for(vit = info.begin(); vit != info.end(); ++vit){
			if((vit->id == new_dit->id) && !(vit->package)){
				if(vit->selected){ // download is selected
					for(int i=0; i<5; i++)
						selection_model->select(index.child(dl_line, i), QItemSelectionModel::Select);

					selected_downloads_size += (double)new_dit->size / 1048576;
					selected_downloads_count++;
					break;
				}
			}
		}

		++old_dit;
		++new_dit;
		dl_line++;
	}

	if(old_dit != old_it->dls.end()){ // there are more old lines than new ones
		while(old_dit != old_it->dls.end()){

			// delete packages out of model
			for(int i=0; i<5; i++){
				dl = pkg->takeChild(dl_line, i);
				delete dl;
			}

			pkg->takeRow(dl_line);

			++old_dit;
		}

	}else if(new_dit != new_it->dls.end()){ // there are more new lines than old ones
		while(new_dit != new_it->dls.end()){
			// insert new lines
		color = build_status(status_text, time_left, *new_dit);

			dl = new QStandardItem(QIcon("img/bullet_black.png"), QString("%1").arg(new_dit->id));
			dl->setEditable(false);
			pkg->setChild(dl_line, 0, dl);

			dl = new QStandardItem(QString(new_dit->title.c_str()));
			dl->setEditable(false);
			pkg->setChild(dl_line, 1, dl);

			dl = new QStandardItem(QString(new_dit->url.c_str()));
			dl->setEditable(false);
			pkg->setChild(dl_line, 2, dl);

			dl = new QStandardItem(QString(time_left.c_str()));
			dl->setEditable(false);
			pkg->setChild(dl_line, 3, dl);

			string colorstring = "img/bullet_" + color + ".png";
			dl = new QStandardItem(QIcon(colorstring.c_str()), trUtf8(status_text.c_str()));
			dl->setEditable(false);
			pkg->setChild(dl_line, 4, dl);

			dl_line++;
			++new_dit;
		}
		list->collapse(index);
		list->expand(index);
	}
}


// slots
void ddclient_gui::on_about(){
	QString build("Build: "); // has to be done here, if not compile time is obsolete
	build += __DATE__;
	build += ", ";
	build += __TIME__;
	build += "\nQt ";
	build += QT_VERSION_STR;

	about_dialog dialog(this, build);
	dialog.setModal(true);
	dialog.exec();

	if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
		this->show();
		this->hide();
	}
}


void ddclient_gui::on_select(){
	if(!check_connection(false))
		return;

	list->expandAll();
	list->selectAll();
}


void ddclient_gui::on_connect(){
	this->show();
	connect_dialog dialog(this, config_dir);
	dialog.setModal(true);
	dialog.exec();

	mx.lock();
	int interval = ((update_thread *)thread)->get_update_interval();
	((update_thread *)thread)->terminate_yourself();
	thread->wait();
	delete thread;

	dclient->set_term(false); // the old thread got terminated, now we have to set term to false again

	update_thread *new_thread = new update_thread(this, interval);
	new_thread->start();
	thread = new_thread;
	this->content.clear();
	mx.unlock();

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

	if(!check_selected()){
		mx.unlock();
		return;
	}

	vector<selected_info>::iterator it;
	int id;

	// make sure user wants to delete downloads
	QMessageBox box(QMessageBox::Question, tsl("Delete Downloads"), tsl("Do you really want to delete\nthe selected Download(s)?"),
					QMessageBox::Yes|QMessageBox::No, this);
	box.setModal(true);
	int del = box.exec();

	if(del != QMessageBox::Yes){ // user clicked no to delete
		mx.unlock();
		return;
	}

	vector<download>::iterator dit;
	int parent_row = -1;
	int dialog_answer = QMessageBox::No; // possible answers are YesToAll, Yes, No, NoToAll

	for(it = selected_lines.begin(); it < selected_lines.end(); it++){

		if(it->package){ // we have a package
			parent_row = it->row;
			id = content.at(parent_row).id;

			// delete every download of that package, then the package
			for(dit = content.at(parent_row).dls.begin(); dit != content.at(parent_row).dls.end(); ++dit){

				try{
					dclient->delete_download(dit->id, dont_know);

				}catch(client_exception &e){
					if(e.get_id() == 7){

						if((dialog_answer != QMessageBox::YesToAll) && (dialog_answer != QMessageBox::NoToAll)){ // file exists and user didn't choose YesToAll or NoToAll before
							stringstream s;
							s << dit->id;
							QMessageBox file_box(QMessageBox::Question, tsl("Delete File"), tsl("Do you want to delete the downloaded File for Download %p1?", s.str().c_str()),
												 QMessageBox::YesToAll|QMessageBox::Yes|QMessageBox::No|QMessageBox::NoToAll, this);
							file_box.setModal(true);
							dialog_answer = file_box.exec();
						}

						if((dialog_answer == QMessageBox::YesToAll) || (dialog_answer == QMessageBox::Yes)){
							try{
								dclient->delete_download(dit->id, del_file);

							}catch(client_exception &e){
								stringstream s;
								s << dit->id;
								QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting File of Download %p1.", s.str().c_str()));
							}

						}else{ // don't delete file
							try{
								dclient->delete_download(dit->id, dont_delete);

							}catch(client_exception &e){
									QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting Download(s)."));
							}
						}
					}else{ // some error occured
						QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting Download(s)."));
					}
				}
			}
			try{
				dclient->delete_package(id);
			}catch(client_exception &e){}

		}else{ // we have a real download
			if(it->parent_row == parent_row) // we already deleted the download because we deleted the whole package
				continue;
			id = content.at(it->parent_row).dls.at(it->row).id;

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

	if(!check_selected()){
		mx.unlock();
		return;
	}

	vector<selected_info>::iterator it;
	string answer;
	int id;

	// make sure user wants to delete files
	QMessageBox box(QMessageBox::Question, tsl("Delete Files"), tsl("Do you really want to delete\nthe selected File(s)?"),
					QMessageBox::Yes|QMessageBox::No, this);
	box.setModal(true);
	int del = box.exec();

	if(del != QMessageBox::Yes){ // user clicked no to delete
		mx.unlock();
		return;
	}

	vector<download>::iterator dit;
	int parent_row = -1;
	for(it = selected_lines.begin(); it < selected_lines.end(); it++){

		if(it->package){ // we have a package
			parent_row = it->row;
			id = content.at(parent_row).id;

			// delete every file of that package
			for(dit = content.at(parent_row).dls.begin(); dit != content.at(parent_row).dls.end(); ++dit){

				try{
					dclient->delete_file(dit->id);

				}catch(client_exception &e){
					if(e.get_id() == 19){ // file error
						stringstream s;
						s << dit->id;
						QMessageBox::warning(this, tsl("Error"), tsl("Error occured at deleting File of Download %p1.", s.str().c_str()));

					}
				}
			}

		}else{ // we have a real download
			 if(it->parent_row == parent_row) // we already deleted the file because we selected the package
				continue;
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
	}

	mx.unlock();
	deselect_lines();
	get_content();
}


void ddclient_gui::on_activate(){
	if(!check_connection(true, "Please connect before activating Downloads."))
		return;

	mx.lock();
	get_selected_lines();

	if(!check_selected()){
		mx.unlock();
		return;
	}

	vector<selected_info>::iterator it;
	int id;
	string error_string;

	vector<download>::iterator dit;
	int parent_row = -1;
	for(it = selected_lines.begin(); it < selected_lines.end(); it++){

	if(it->package){ // we have a package
		parent_row = it->row;
		id = content.at(parent_row).id;

		// activate every download of that package
		for(dit = content.at(parent_row).dls.begin(); dit != content.at(parent_row).dls.end(); ++dit){

			try{
				dclient->activate_download(dit->id);

			}catch(client_exception &e){}
		}

	}else{ // we have a real download
		 if(it->parent_row == parent_row) // we already activated the donwload because we selected the package
			continue;
		id = content.at(it->parent_row).dls.at(it->row).id;

		try{
			dclient->activate_download(id);
		}catch(client_exception &e){}
		}
	}

	mx.unlock();
	get_content();
}


void ddclient_gui::on_deactivate(){
	if(!check_connection(true, "Please connect before deactivating Downloads."))
		return;

	mx.lock();
	get_selected_lines();

	if(!check_selected()){
		mx.unlock();
		return;
	}

	vector<selected_info>::iterator it;
	int id;
	string error_string;

	vector<download>::iterator dit;
	int parent_row = -1;
	for(it = selected_lines.begin(); it < selected_lines.end(); it++){

	if(it->package){ // we have a package
		parent_row = it->row;
		id = content.at(parent_row).id;

		// activate every download of that package
		for(dit = content.at(parent_row).dls.begin(); dit != content.at(parent_row).dls.end(); ++dit){

			try{
				dclient->deactivate_download(dit->id);

			}catch(client_exception &e){}
		}

	}else{ // we have a real download
		 if(it->parent_row == parent_row) // we already activated the donwload because we selected the package
			continue;
		id = content.at(it->parent_row).dls.at(it->row).id;

		try{
			dclient->deactivate_download(id);
		}catch(client_exception &e){}
		}
	}

	mx.unlock();
	get_content();
}


void ddclient_gui::on_priority_up(){
	if(!check_connection(true, "Please connect before increasing Priority."))
		return;

	mx.lock();
	get_selected_lines();

	if(!check_selected()){
		mx.unlock();
		return;
	}

	vector<selected_info>::iterator it;
	int id;

	for(it = selected_lines.begin(); it<selected_lines.end(); it++){

		if(!(it->package)) // we have a real download
			id = content.at(it->parent_row).dls.at(it->row).id;
		else // we have a package selected
			id = content.at(it->row).id;

		try{
			if(it->package)
				dclient->package_priority_up(id);
			else
				dclient->priority_up(id);
		}catch(client_exception &e){}
	}

	mx.unlock();
	get_content();
}


void ddclient_gui::on_priority_down(){
	if(!check_connection(true, "Please connect before decreasing Priority."))
		return;

	mx.lock();
	get_selected_lines();

	if(!check_selected()){
		mx.unlock();
		return;
	}

	vector<selected_info>::reverse_iterator rit;
	int id;
	string error_string;

	for(rit = selected_lines.rbegin(); rit<selected_lines.rend(); rit++){

		if(!(rit->package)) // we have a real download
			id = content.at(rit->parent_row).dls.at(rit->row).id;
		else // we have a package selected
			id = content.at(rit->row).id;

		try{
			if(rit->package)
				dclient->package_priority_down(id);
			else
				dclient->priority_down(id);
		}catch(client_exception &e){}
	}

	mx.unlock();
	get_content();
}


void ddclient_gui::on_enter_captcha(){
	if(!check_connection(true, "Please connect before entering a Captcha."))
		return;

	mx.lock();
	get_selected_lines();

	if(!check_selected()){
		mx.unlock();
		return;
	}

	vector<selected_info>::iterator it;
	int id;

	for(it = selected_lines.begin(); it<selected_lines.end(); it++){

		if(!(it->package)) // we have a real download
			id = content.at(it->parent_row).dls.at(it->row).id;
		else // we have a package selected
			continue;

		string type, question;

		try{
			string image = dclient->get_captcha(id, type, question);
			if(image == "")
				continue;

			// Create temporary image
			fstream captcha_image(string("/tmp/captcha." + type).c_str(), fstream::out);
			captcha_image << image;
			captcha_image.close();

			captcha_dialog dialog(this, "/tmp/captcha." + type, question, id);
			dialog.setModal(true);
			dialog.exec();

			if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
				this->show();
				this->hide();
			}
		}catch(client_exception &e){}
	}

	mx.unlock();
	get_content();
}

void ddclient_gui::on_configure(){
	if(!check_connection(true, "Please connect before configurating DownloadDaemon."))
		return;

	configure_dialog dialog(this, config_dir);
	dialog.setModal(true);
	dialog.exec();

	if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
		this->show();
		this->hide();
	}
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
	configure_menu->removeAction(activate_action);
	configure_menu->removeAction(deactivate_action); // just to be save
	configure_menu->addAction(deactivate_action);

	if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
		this->show();
		this->hide();
	}
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
	configure_menu->removeAction(activate_action);
	configure_menu->removeAction(deactivate_action); // just to be save
	configure_menu->addAction(activate_action);

	if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
		this->show();
		this->hide();
	}
}


void ddclient_gui::on_copy(){
	if(!check_connection(true, "Please connect before copying URLs."))
		return;

	mx.lock();
	get_selected_lines();

	if(!check_selected()){
		mx.unlock();
		return;
	}

	vector<selected_info>::iterator it;
	QString clipboard_data;
	vector<download>::iterator dit;
	int parent_row = -1;
	for(it = selected_lines.begin(); it < selected_lines.end(); it++){

		if(it->package){ // we have a package
			parent_row = it->row;

			// copy every url of that package
			for(dit = content.at(parent_row).dls.begin(); dit != content.at(parent_row).dls.end(); ++dit){
				clipboard_data += dit->url.c_str();
				clipboard_data += '\n';
			}

		}else{ // we have a real download
			 if(it->parent_row == parent_row) // we already copied the url because we selected the package
				continue;

			 clipboard_data += content.at(it->parent_row).dls.at(it->row).url.c_str();
			 clipboard_data += '\n';
		}
	}

	QApplication::clipboard()->setText(clipboard_data);
	mx.unlock();
}


void ddclient_gui::on_paste(){
	if(!check_connection(true, "Please connect before adding Downloads."))
		return;

	string text = QApplication::clipboard()->text().toStdString();
	mx.lock();

	bool error_occured = false;
	size_t lineend = 1, urlend;
	int package = -1, error = 0;
	string line, url, title;

	// parse lines
	while(text.length() > 0 && lineend != string::npos){
		lineend = text.find("\n"); // termination character for line

		if(lineend == string::npos){ // this is the last line (which ends without \n)
			line = text.substr(0, text.length());
			text = "";

		}else{ // there is still another line after this one
			line = text.substr(0, lineend);
			text = text.substr(lineend+1);
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
			if(package == -1) // create a new package
				package = dclient->add_package();

			dclient->add_download(package, url, title);
		}catch(client_exception &e){
			error_occured = true;
			error = e.get_id();
		}
	}

	mx.unlock();

	if(error_occured){
		if(error == 6)
			QMessageBox::warning(this,  tsl("Error"), tsl("Failed to create Package."));
		else if(error == 13)
			QMessageBox::warning(this,  tsl("Invalid URL"), tsl("At least one inserted URL was invalid."));

		if(!this->isVisible()){ // workaround: if the user closes an error message and there is no window visible the whole programm closes!
			this->show();
			this->hide();
		}
	}
}


void ddclient_gui::on_set_password(){
	if(!check_connection(true, "Please connect before changing Packages."))
		return;

	mx.lock();
	get_selected_lines();

	if(!check_selected()){
		mx.unlock();
		return;
	}

	vector<selected_info>::iterator it;
	string answer;
	int id;

	for(it = selected_lines.begin(); it < selected_lines.end(); it++){

		if(it->package){ // we have a package
			id = content.at(it->row).id;

			bool ok;
		string old_pass = dclient->get_package_var(id, "PKG_PASSWORD");

		QString pass = QInputDialog::getText(this, tsl("Enter Package Password"), tsl("Enter Package Password"), QLineEdit::Normal, old_pass.c_str(), &ok);
			if(!ok){
				mx.unlock();
				return;
			}

			try{
				dclient->set_package_var(id, "PKG_PASSWORD", pass.toStdString());
			}catch(client_exception &e){}

		}else{} // we have a real download, but we don't need it
	}

	mx.unlock();
}


void ddclient_gui::on_set_name(){
	if(!check_connection(true, "Please connect before changing the Download List."))
		return;

	mx.lock();
	get_selected_lines();

	if(!check_selected()){
		mx.unlock();
		return;
	}

	vector<selected_info>::iterator it;
	string answer;
	int id;

	for(it = selected_lines.begin(); it < selected_lines.end(); it++){

		if(it->package){ // we have a package
			id = content.at(it->row).id;

			bool ok;
			stringstream s;
			s << id;
			QString name = QInputDialog::getText(this, tsl("Enter Title"), tsl("Enter Title of Package %p1", s.str().c_str()), QLineEdit::Normal, "", &ok);
			if(!ok){
				mx.unlock();
				return;
			}

			try{
				dclient->set_package_var(id, "PKG_NAME", name.toStdString());
			}catch(client_exception &e){}

		}else{ // we have a real download
			id = content.at(it->parent_row).dls.at(it->row).id;

			bool ok;
			stringstream s;
			s << id;
			QString name = QInputDialog::getText(this, tsl("Enter Title"), tsl("Enter Title of Download %p1", s.str().c_str()), QLineEdit::Normal, "", &ok);
			if(!ok){
				mx.unlock();
				return;
			}

			try{
				dclient->set_download_var(id, "DL_TITLE", name.toStdString());
			}catch(client_exception &e){
				if(e.get_id() == 18)
					QMessageBox::information(this, tsl("Error"), tsl("Running or finished Downloads can't be changed."));
			}
		}
	}

	mx.unlock();
}


void ddclient_gui::on_set_url(){
	if(!check_connection(true, "Please connect before changing the Download List."))
		return;

	mx.lock();
	get_selected_lines();

	if(!check_selected()){
		mx.unlock();
		return;
	}

	vector<selected_info>::iterator it;
	string answer;
	int id;

	for(it = selected_lines.begin(); it < selected_lines.end(); it++){

		if(it->package){ // we have a package, but don't need it
		}else{ // we have a real download
			id = content.at(it->parent_row).dls.at(it->row).id;

			bool ok;
			stringstream s;
			s << id;
			QString url = QInputDialog::getText(this, tsl("Enter URL"), tsl("Enter URL of Download %p1", s.str().c_str()), QLineEdit::Normal, "", &ok);
			if(!ok){
				mx.unlock();
				return;
			}

			try{
				dclient->set_download_var(id, "DL_URL", url.toStdString());
			}catch(client_exception &e){
				if(e.get_id() == 18)
					QMessageBox::information(this, tsl("Error"), tsl("Running or finished Downloads can't be changed."));
			}
		}
	}

	mx.unlock();
}


void ddclient_gui::on_load_container(){
	if(!check_connection(true, "Please connect before adding Containers."))
		return;

	QFileDialog dialog(this,tsl("Add Download Container"), "", "*.rsdf ; *.dlc");
	dialog.setModal(true);
	dialog.setFileMode(QFileDialog::ExistingFiles);

	QStringList file_names;
	if(dialog.exec() == QDialog::Accepted )
		file_names = dialog.selectedFiles();

	mx.lock();
	for (int i = 0; i < file_names.size(); ++i){ // loop every file name

	fstream f;
	string fn = file_names.at(i).toStdString();
	f.open(fn.c_str(), fstream::in);
	string content;
	for(std::string tmp; getline(f, tmp); content += tmp); // read data into string

		try{
			if (fn.rfind("rsdf") == fn.size() - 4 || fn.rfind("RSDF") == fn.size() - 4)
				dclient->pkg_container("RSDF", content);
			else
				dclient->pkg_container("DLC", content);
		}catch(client_exception &e){}
	}

	mx.unlock();
}


void ddclient_gui::on_activate_tray_icon(QSystemTrayIcon::ActivationReason reason){
	switch (reason) {
	case QSystemTrayIcon::Trigger:
	case QSystemTrayIcon::DoubleClick:
		if(this->isVisible())
			this->hide();
		else
			this->show();

		//((update_thread *) thread)->toggle_updating(); // sets updating on or off, sadly this makes the tooltip a bit useless
		break;
	default:
		;

	}
}


void ddclient_gui::on_reload(){
	if(!check_connection()){
		status_connection->setText(tsl("Not connected"));
		list_model->setRowCount(0);
		mx.lock();
		content.clear();
		mx.unlock();
		return;
	}

	mx.lock();

	download_speed = 0;
	not_downloaded_yet = 0;
	selected_downloads_size = 0;
	selected_downloads_count = 0;

	// update download list
	if(full_list_update){ // normal update mode without subscriptions

		if(content.size() == 0) // the content gets deleted whenever the language changes
			list_model->setRowCount(0);

		compare_packages();

		content.clear();
		content = new_content;
		full_list_update = false;
	}else{ // subscription update mode => less cpu usage
		update_packages();

		if(reload_list)
			get_content();
	}

	int package_count = this->content.size(); // todo
	// update statusbar
	if(selected_downloads_size != 0){ // something is selected and the total size is known
		status_connection->setText(tsl("Connected to") + " " + server + " | " + tsl("Selected Size") + ": " + QString("%1 MB").arg(selected_downloads_size) + ", " + QString("%1").arg(selected_downloads_count) + " " + tsl("Download(s)"));

		if(QSystemTrayIcon::isSystemTrayAvailable())
						tray_icon->setToolTip(tsl("Connected to") + " " + server + "\n" + tsl("Selected Size") + ": " + QString("%1 MB").arg(selected_downloads_size) + ", " + QString("%1").arg(selected_downloads_count) + " " + tsl("Download(s)"));

	}else{

		string time_left;

		if(download_speed > 0){
			stringstream s;
			s << (int)((not_downloaded_yet*1024)/download_speed);
			time_left = s.str();
			ddclient_gui::cut_time(time_left);

		}else
			time_left = "-";

		stringstream speed;
		speed << setprecision(1) << fixed << download_speed << " kb/s";

		status_connection->setText(tsl("Connected to") + " " + server + " | " + tsl("Total Speed") + ": " + speed.str().c_str() +
								   " | " + tsl("Pending Queue Size") + ": " + QString("%1 MB").arg(not_downloaded_yet) + " | " + tsl("Time left") + ": " + time_left.c_str() + " packages: " + QString("%1 Packages").arg(package_count)); // todo
		if(QSystemTrayIcon::isSystemTrayAvailable())
						tray_icon->setToolTip(tsl("Connected to") + " " + server + "\n" + tsl("Total Speed") + ": " + speed.str().c_str() +
							  "\n" + tsl("Pending Queue Size") + ": " + QString("%1 MB").arg(not_downloaded_yet) + "\n" + tsl("Time left") + ": " + time_left.c_str());
	}

	mx.unlock();
}

void ddclient_gui::donate_flattr(){
	QDesktopServices::openUrl(QUrl(QString("http://flattr.com/thing/65487/DownloadDaemon")));
}

void ddclient_gui::donate_sf(){
	QDesktopServices::openUrl(QUrl(QString("http://sourceforge.net/donate/index.php?group_id=278029")));
}

void ddclient_gui::contextMenuEvent(QContextMenuEvent *event){
	QMenu menu(this);

	QAction* delete_file_action = new QAction(QIcon("img/15_delete_file.png"), tsl("Delete File"), this);
	delete_file_action->setShortcut(QString("Ctrl+F"));
	delete_file_action->setStatusTip(tsl("Delete File"));

	QAction* set_password_action = new QAction(QIcon("img/package.png"), tsl("Enter Package Password"), this);
	set_password_action->setStatusTip(tsl("Enter Package Password"));

	QAction* set_name_action = new QAction(QIcon("img/download_package.png"), tsl("Enter Title"), this);
	set_name_action->setStatusTip(tsl("Enter Title"));

	QAction* set_url_action = new QAction(QIcon("img/bullet_black.png"), tsl("Enter URL"), this);
	set_url_action->setStatusTip(tsl("Enter URL"));

	menu.addAction(container_action);
	menu.addSeparator();
	menu.addAction(activate_download_action);
	menu.addAction(deactivate_download_action);
	menu.addSeparator();
	menu.addAction(delete_action);
	menu.addAction(delete_file_action);
	menu.addSeparator();
	menu.addAction(select_action);
	menu.addAction(copy_action);
	menu.addSeparator();
	menu.addAction(set_password_action);
	menu.addAction(set_name_action);
	menu.addAction(set_url_action);
	menu.addAction(captcha_action);

	connect(delete_file_action, SIGNAL(triggered()), this, SLOT(on_delete_file()));
	connect(set_password_action, SIGNAL(triggered()), this, SLOT(on_set_password()));
	connect(set_name_action, SIGNAL(triggered()), this, SLOT(on_set_name()));
	connect(set_url_action, SIGNAL(triggered()), this, SLOT(on_set_url()));

	menu.exec(event->globalPos());
}

void ddclient_gui::resizeEvent(QResizeEvent* event){
	event = event;
	double width = list->width();

	width -= 250;
	list->setColumnWidth(0, 100); // fixed sizes
	list->setColumnWidth(3, 100);

	if(width > 600){ // use different ratio if window is a bit bigger then normal
		list->setColumnWidth(4, 300);
		width -= 300;
		list->setColumnWidth(1, 0.3*width);
		list->setColumnWidth(2, 0.7*width);
	}else{
		list->setColumnWidth(1, 0.2*width);
		list->setColumnWidth(2, 0.3*width);
		list->setColumnWidth(4, 0.55*width);
	}
}
