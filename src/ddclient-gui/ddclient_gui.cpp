#include "ddclient_gui.h"
#include <QtGui/QStatusBar>
#include <QtGui/QMenuBar>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QLabel>
#include <QToolBar>
#include <QStringList>
#include <QStandardItem>
#include <QtGui/QContextMenuEvent>
//#include <QtGui> // this is only for testing if includes are the problem

#include <sstream>

using namespace std;

ddclient_gui::ddclient_gui(QString config_dir) : QMainWindow(NULL), conf_dir(config_dir) {
    setWindowTitle("DownloadDaemon Client GUI");
    this->resize(750, 500);
    setWindowIcon(QIcon("IMG/logoDD.png"));                // have to find working dir again

    statusBar()->show();
    status_connection = new QLabel(tsl("Not connected"));
    statusBar()->insertPermanentWidget(0, status_connection);

    add_bars();

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

    // testdata
    QStandardItem *testitem1 = new QStandardItem(QIcon("img/package.png"), tsl("0"));
    QStandardItem *testitem2 = new QStandardItem(tsl("Packettitel1"));
    QStandardItem *testitem3 = new QStandardItem(QIcon("img/bullet_black.png"), tsl("1"));
    QStandardItem *testitem8 = new QStandardItem(tsl("10h, 59:25m"));
    QStandardItem *testitem4 = new QStandardItem(tsl("Downloadtitel1"));
    QStandardItem *testitem5 = new QStandardItem(QIcon("img/bullet_black.png"), tsl("2"));
    QStandardItem *testitem6 = new QStandardItem(QIcon("img/package.png"), tsl("1"));
    QStandardItem *testitem7 = new QStandardItem(QIcon("img/bullet_black.png"), tsl("3"));

    testitem1->setEditable(false);
    testitem2->setEditable(false);
    testitem3->setEditable(false);
    testitem4->setEditable(false);
    testitem5->setEditable(false);
    testitem6->setEditable(false);
    testitem7->setEditable(false);

    testitem1->setChild(0, testitem3);
    testitem1->setChild(0, 1, testitem4);
    testitem1->setChild(1, 0, testitem5);
    testitem1->setChild(0, 3, testitem8);

    testitem6->setChild(0, testitem7);

    list_model->setItem(0, 0, testitem1);
    list_model->setItem(0, 1, testitem2);
    list_model->setItem(1, 0, testitem6);
}


ddclient_gui::~ddclient_gui(){
}


QString ddclient_gui::tsl(string text, ...){
    string translated = text; // = lang[text];            TODO: get language working (need working dir)

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


void ddclient_gui::add_bars(){
    // menubar
    file_menu = menuBar()->addMenu("&" + tsl("File"));
    QMenu* help_menu = menuBar()->addMenu("&" + tsl("Help"));

    QAction* connect_action = new QAction(QIcon("img/1_connect.png"), "&" + tsl("Connect"), this);
    connect_action->setShortcut(QString("Alt+C"));
    connect_action->setStatusTip(tsl("Connect to a DownloadDaemon Server"));
    file_menu->addAction(connect_action);

    QAction* configure_action = new QAction(QIcon("img/8_configure.png"), tsl("Configure"), this);
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

    QAction* activate_download_action = new QAction(QIcon("img/5_start.png"), tsl("Activate Download"), this);
    activate_download_action->setShortcut(QString("Alt+A"));
    activate_download_action->setStatusTip(tsl("Activate the selected Download"));
    file_menu->addAction(activate_download_action);

    QAction* deactivate_download_action = new QAction(QIcon("img/4_stop.png"), tsl("Deactivate Download"), this);
    deactivate_download_action->setShortcut(QString("Alt+D"));
    deactivate_download_action->setStatusTip(tsl("Deactivate the selected Download"));
    file_menu->addAction(deactivate_download_action);
    file_menu->addSeparator();

    QAction* add_action = new QAction(QIcon("img/2_add.png"), tsl("Add Download"), this);
    add_action->setShortcut(QString("Alt+I"));
    add_action->setStatusTip(tsl("Add Download"));
    file_menu->addAction(add_action);

    QAction* delete_action = new QAction(QIcon("img/3_delete.png"), tsl("Delete Download"), this);
    delete_action->setShortcut(QString("DEL"));
    delete_action->setStatusTip(tsl("Delete Download"));
    file_menu->addAction(delete_action);

    QAction* delete_finished_action = new QAction(QIcon("img/10_delete_finished.png"), tsl("Delete finished Downloads"), this);
    delete_finished_action->setShortcut(QString("Ctrl+DEL"));
    delete_finished_action->setStatusTip(tsl("Delete finished Downloads"));
    file_menu->addAction(delete_finished_action);
    file_menu->addSeparator();

    QAction* select_action = new QAction(tsl("Select all"), this);
    select_action->setShortcut(QString("Ctrl+A"));
    select_action->setStatusTip(tsl("Select all"));
    file_menu->addAction(select_action);

    QAction* copy_action = new QAction(tsl("Copy URL"), this);
    copy_action->setShortcut(QString("Ctrl+C"));
    copy_action->setStatusTip(tsl("Copy URL"));
    file_menu->addAction(copy_action);
    file_menu->addSeparator();

    QAction* quit_action = new QAction("&" + tsl("Quit"), this);
    quit_action->setShortcut(QString("Alt+F4"));
    quit_action->setStatusTip(tsl("Quit"));
    file_menu->addAction(quit_action);

    QAction* about_action = new QAction("&" + tsl("About"), this);
    about_action->setShortcut(QString("F1"));
    about_action->setStatusTip(tsl("About"));
    help_menu->addAction(about_action);

    // toolbar
    QAction* up_action = new QAction(QIcon("img/6_up.png"), "&" + tsl("Increase Priority"), this);
    about_action->setStatusTip(tsl("Increase Priority of the selected Download"));

    QAction* down_action = new QAction(QIcon("img/7_down.png"), "&" + tsl("Dencrease Priority"), this);
    about_action->setStatusTip(tsl("Decrease Priority of the selected Download"));

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


// slots
void ddclient_gui::on_about(){
    QMessageBox::information(this, "Test", "on_about");
}


void ddclient_gui::on_select(){
    QMessageBox::information(this, "Test", "on_select");
    list->expandAll();
    list->selectAll();

}


void ddclient_gui::on_connect(){
    QMessageBox::information(this, "Test", "on_connect");
}


void ddclient_gui::on_add(){
    QMessageBox::information(this, "Test", "on_add");
}


void ddclient_gui::on_delete(){
    QMessageBox::information(this, "Test", "on_delete");
}


void ddclient_gui::on_delete_finished(){
    QMessageBox::information(this, "Test", "on_delete_finished");
}


void ddclient_gui::on_delete_file(){
    QMessageBox::information(this, "Test", "on_delete_file");
}


void ddclient_gui::on_activate(){
    QMessageBox::information(this, "Test", "on_activate");
}


void ddclient_gui::on_deactivate(){
    QMessageBox::information(this, "Test", "on_deactivate");
}


void ddclient_gui::on_priority_up(){
    QMessageBox::information(this, "Test", "on_priority_up");
}


void ddclient_gui::on_priority_down(){
    QMessageBox::information(this, "Test", "on_priority_down");
}


void ddclient_gui::on_configure(){
    QMessageBox::information(this, "Test", "on_configure");
}


void ddclient_gui::on_downloading_activate(){
    QMessageBox::information(this, "Test", "on_downloading_activate");
}


void ddclient_gui::on_downloading_deactivate(){
    QMessageBox::information(this, "Test", "on_downloading_deactivate");
}


void ddclient_gui::on_copy(){
    QMessageBox::information(this, "Test", "on_copy");
}


void ddclient_gui::on_reload(){
    QMessageBox::information(this, "Test", "on_reload");
}


void ddclient_gui::contextMenuEvent(QContextMenuEvent *event){
    QMenu menu(this);

    QAction* activate_download_action = new QAction(tsl("Activate Download"), this);
    activate_download_action->setShortcut(QString("Alt+A"));
    activate_download_action->setStatusTip(tsl("Activate Download"));
    menu.addAction(activate_download_action);

    QAction* deactivate_download_action = new QAction(tsl("Deactivate Download"), this);
    deactivate_download_action->setShortcut(QString("Alt+D"));
    deactivate_download_action->setStatusTip(tsl("Deactivate Download"));
    menu.addAction(deactivate_download_action);
    menu.addSeparator();

    QAction* delete_action = new QAction(tsl("Delete Download"), this);
    delete_action->setShortcut(QString("DEL"));
    delete_action->setStatusTip(tsl("Delete Download"));
    menu.addAction(delete_action);

    QAction* delete_file_action = new QAction(tsl("Delete File"), this);
    delete_file_action->setShortcut(QString("Ctrl+F"));
    delete_file_action->setStatusTip(tsl("Delete File"));
    menu.addAction(delete_file_action);
    menu.addSeparator();

    QAction* select_action = new QAction(tsl("Select all"), this);
    select_action->setShortcut(QString("Ctrl+A"));
    select_action->setStatusTip(tsl("Select all"));
    menu.addAction(select_action);

    QAction* copy_action = new QAction(tsl("Copy URL"), this);
    copy_action->setShortcut(QString("Ctrl+C"));
    copy_action->setStatusTip(tsl("Copy URL"));
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
