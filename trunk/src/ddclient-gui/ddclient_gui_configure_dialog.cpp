#include "ddclient_gui_configure_dialog.h"
#include "ddclient_gui.h"
#include <string>
#include <fstream>

#include <QtGui/QFormLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QDialogButtonBox>
#include <QtGui/QMessageBox>
#include <QTabWidget>
#include <QStringList>

using namespace std;


configure_dialog::configure_dialog(QWidget *parent) : QDialog(parent){
    ddclient_gui *p = (ddclient_gui *) parent;

    setWindowTitle(p->tsl("Configure DownloadDaemon"));

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addStrut(450);
    setLayout(layout);
    QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    QTabWidget *tabs = new QTabWidget();
    tabs->addTab (create_general_panel(), p->tsl("General"));
    tabs->addTab (create_download_panel(), p->tsl("Download"));
    tabs->addTab (create_password_panel(), p->tsl("Password"));
    tabs->addTab (create_logging_panel(), p->tsl("Logging"));
    tabs->addTab (create_reconnect_panel(), p->tsl("Reconnect"));

    layout->addWidget(tabs);
    layout->addWidget(button_box);

    button_box->button(QDialogButtonBox::Ok)->setDefault(true);
    button_box->button(QDialogButtonBox::Ok)->setFocus(Qt::OtherFocusReason);

    connect(button_box->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(ok()));
    connect(button_box->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
}


QWidget *configure_dialog::create_general_panel(){
    ddclient_gui *p = (ddclient_gui *) this->parent();
    downloadc *dclient = p->get_connection();

    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    page->setLayout(layout);

    QGroupBox *premium_group_box = new QGroupBox(p->tsl("Premium Account"));
    QGroupBox *general_group_box = new QGroupBox(p->tsl("General Options"));
    QFormLayout *form_layout = new QFormLayout();
    QFormLayout *general_layout = new QFormLayout();
    premium_group_box->setLayout(form_layout);
    general_group_box->setLayout(general_layout);

    // premium host
    premium_host = new QComboBox();
    vector<string> host_list;
    string line = "";

    QMutex *mx = p->get_mutex();
    mx->lock();

    try{
        host_list = dclient->get_premium_list();
    }catch(client_exception &e){}

    mx->unlock();

    // parse lines
    vector<string>::iterator it = host_list.begin();

    for(; it != host_list.end(); it++){
        premium_host->addItem(it->c_str());
    }

    // other premium items
    premium_user = new QLineEdit();
    premium_password = new QLineEdit();
    premium_password->setEchoMode(QLineEdit::Password);
    QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Save);

    form_layout->addRow(p->tsl("Host"), premium_host);
    form_layout->addRow(p->tsl("Username"), premium_user);
    form_layout->addRow(p->tsl("Password"), premium_password);
    form_layout->addRow("", button_box);

    layout->addWidget(premium_group_box);

    // general options
    bool enable = false;
    if(get_var("overwrite_files") == "1")
        enable = true;

    overwrite = new QCheckBox(p->tsl("overwrite existing Files"));
    overwrite->setChecked(enable);

    enable = false;
    if(get_var("refuse_existing_links") == "1")
        enable = true;

    refuse_existing = new QCheckBox(p->tsl("refuse existing Links"));
    refuse_existing->setChecked(enable);

    general_layout->addRow("", overwrite);
    general_layout->addRow("", refuse_existing);

    connect(button_box->button(QDialogButtonBox::Save), SIGNAL(clicked()),this, SLOT(save_premium()));

    layout->addWidget(general_group_box);
    return page;
}


QWidget *configure_dialog::create_download_panel(){
    ddclient_gui *p = (ddclient_gui *) this->parent();

    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    page->setLayout(layout);

    QGroupBox *time_group_box = new QGroupBox(p->tsl("Download Time"));
    QGroupBox *folder_group_box = new QGroupBox(p->tsl("Download Folder"));
    QGroupBox *options_group_box = new QGroupBox(p->tsl("Additional Download Options"));
    QVBoxLayout *t_layout = new QVBoxLayout();
    QFormLayout *f_layout = new QFormLayout();
    QVBoxLayout *o_layout = new QVBoxLayout();
    time_group_box->setLayout(t_layout);
    folder_group_box->setLayout(f_layout);
    options_group_box->setLayout(o_layout);
    QHBoxLayout *inner_t_layout = new QHBoxLayout();
    QHBoxLayout *inner_o_layout = new QHBoxLayout();

    start_time = new QLineEdit(get_var("download_timing_start"));
    end_time = new QLineEdit(get_var("download_timing_end"));
    folder = new QLineEdit(get_var("download_folder"));
    count = new QLineEdit(get_var("simultaneous_downloads"));
    count->setFixedWidth(50);
    speed = new QLineEdit(get_var("max_dl_speed"));
    speed->setFixedWidth(50);

    t_layout->addWidget(new QLabel(p->tsl("You can force DownloadDaemon to only download at specific times by entering a start and"
                                                "\nend time in the format hours:minutes."
                                                "\nLeave these fields empty if you want to allow DownloadDaemon to download permanently.")));
    t_layout->addLayout(inner_t_layout);
    inner_t_layout->addWidget(new QLabel(p->tsl("Start Time")));
    inner_t_layout->addWidget(start_time);
    inner_t_layout->addWidget(new QLabel(p->tsl("End Time")));
    inner_t_layout->addWidget(end_time);

    f_layout->addRow("", new QLabel(p->tsl("This option specifies where finished downloads should be safed on the server.")));
    f_layout->addRow("", folder);

    o_layout->addWidget(new QLabel(p->tsl("Here you can specify how many downloads may run at the same time and regulate the"
                                          "\ndownload speed for each download (overall max speed is download number * max speed).")));
    o_layout->addLayout(inner_o_layout);
    inner_o_layout->addWidget(new QLabel(p->tsl("Simultaneous Downloads")));
    inner_o_layout->addWidget(count);
    inner_o_layout->addWidget(new QLabel(p->tsl("Maximal Speed in kb/s")));
    inner_o_layout->addWidget(speed);

    layout->addWidget(time_group_box);
    layout->addWidget(folder_group_box);
    layout->addWidget(options_group_box);
    return page;
}


QWidget *configure_dialog::create_password_panel(){
    ddclient_gui *p = (ddclient_gui *) this->parent();

    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    page->setLayout(layout);

    QGroupBox *group_box = new QGroupBox(p->tsl("Change Password"));
    QFormLayout *form_layout = new QFormLayout();
    group_box->setLayout(form_layout);

    old_password = new QLineEdit();
    old_password->setEchoMode(QLineEdit::Password);
    new_password = new QLineEdit();
    new_password->setEchoMode(QLineEdit::Password);
    QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Save);

    form_layout->addRow(p->tsl("Old Password"), old_password);
    form_layout->addRow(p->tsl("New Password"), new_password);
    form_layout->addRow("", button_box);

    connect(button_box->button(QDialogButtonBox::Save), SIGNAL(clicked()),this, SLOT(save_password()));

    layout->addWidget(group_box);
    return page;
}


QWidget *configure_dialog::create_logging_panel(){
    ddclient_gui *p = (ddclient_gui *) this->parent();

    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    page->setLayout(layout);

    QGroupBox *procedure_group_box = new QGroupBox(p->tsl("Log Procedure"));
    QGroupBox *activity_group_box = new QGroupBox(p->tsl("Logging Activity"));
    QFormLayout *p_form_layout = new QFormLayout();
    QFormLayout *a_form_layout = new QFormLayout();
    procedure_group_box->setLayout(p_form_layout);    
    activity_group_box->setLayout(a_form_layout);

    // log procedure
    procedure = new QComboBox();
    procedure->addItem(p->tsl("stdout - Standard output"));
    procedure->addItem(p->tsl("stderr - Standard error output"));
    procedure->addItem(p->tsl("syslog - Syslog-daemon"));

    QString old_output = get_var("log_procedure");

    if(old_output == "stdout")
        procedure->setCurrentIndex(0);
    else if(old_output == "stderr")
        procedure->setCurrentIndex(1);
    else if(old_output == "syslog")
        procedure->setCurrentIndex(2);

    // log activity
    activity = new QComboBox();
    activity->addItem(p->tsl("Debug"));
    activity->addItem(p->tsl("Warning"));
    activity->addItem(p->tsl("Severe"));
    activity->addItem(p->tsl("Off"));

    QString old_activity = get_var("log_level");

    if(old_activity == "DEBUG")
        activity->setCurrentIndex(0);
    else if(old_activity == "WARNING")
        activity->setCurrentIndex(1);
    else if(old_activity == "SEVERE")
        activity->setCurrentIndex(2);
    else if(old_activity == "OFF")
        activity->setCurrentIndex(3);

    p_form_layout->addRow("", new QLabel(p->tsl("This option specifies how logging should be done\n(Standard output, Standard error"
                                                "output, Syslog-daemon).")));
    p_form_layout->addRow("", procedure);
    a_form_layout->addRow("", new QLabel(p->tsl("This option specifies DownloadDaemons logging activity.")));
    a_form_layout->addRow("", activity);

    layout->addWidget(procedure_group_box);
    layout->addWidget(activity_group_box);
    return page;
}


QWidget *configure_dialog::create_reconnect_panel(){
    ddclient_gui *p = (ddclient_gui *) this->parent();
    downloadc *dclient = p->get_connection();

    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    page->setLayout(layout);

    reconnect_group_box = new QGroupBox(p->tsl("enable reconnecting"));
    reconnect_group_box->setCheckable(true);
    bool enable = false;
    if(get_var("enable_reconnect") == "1")
        enable = true;
    reconnect_group_box->setChecked(enable);
    QFormLayout *form_layout = new QFormLayout();
    reconnect_group_box->setLayout(form_layout);

    QHBoxLayout *policy_layout = new QHBoxLayout();
    QWidget *policy_widget = new QWidget();
    policy_widget->setLayout(policy_layout);

    // reconnect policy
    reconnect_policy = new QComboBox();
    reconnect_policy->addItem(p->tsl("Hard"));
    reconnect_policy->addItem(p->tsl("Continue"));
    reconnect_policy->addItem(p->tsl("Soft"));
    reconnect_policy->addItem(p->tsl("Pussy"));
    QString old_policy = get_var("reconnect_policy", ROUTER_T);

    if(old_policy == "HARD")
        reconnect_policy->setCurrentIndex(0);
    else if(old_policy == "CONTINUE")
        reconnect_policy->setCurrentIndex(1);
    else if(old_policy == "SOFT")
        reconnect_policy->setCurrentIndex(2);
    else if(old_policy == "PUSSY")
        reconnect_policy->setCurrentIndex(3);

    QPushButton *help_button = new QPushButton("?");
    help_button->setFixedWidth(20);
    policy_layout->addWidget(reconnect_policy);
    policy_layout->addWidget(help_button);

    // router model
    model = new QListWidget();
    model->setFixedHeight(100);
    QStringList model_input;
    string line = "", old_model;
    int selection = 0, i = 0;
    vector<string> model_list;

    old_model = (get_var("router_model", ROUTER_T)).toStdString();
    model_input << "";
    router_model_list.push_back("");

    QMutex *mx = p->get_mutex();
    mx->lock();

    try{
        model_list = dclient->get_router_list();
    }catch(client_exception &e){}

    mx->unlock();

    // parse lines
    vector<string>::iterator it = model_list.begin();

    for(; it != model_list.end(); it++){
        model_input << it->c_str();
        router_model_list.push_back(*it);

        i++;
        if(*it == old_model)
            selection = i;
    }

    model->addItems(model_input);
    QListWidgetItem *sel_item = model->item(selection);
    sel_item->setSelected(true);

    // router data
    model_search = new QLineEdit();
    ip = new QLineEdit(get_var("router_ip", ROUTER_T));
    username = new QLineEdit(get_var("router_username", ROUTER_T));
    password = new QLineEdit();
    password->setEchoMode(QLineEdit::Password);

    form_layout->addRow(p->tsl("Reconnect Policy"), policy_widget);
    form_layout->addRow(p->tsl("Model"), model_search);
    form_layout->addRow("", model);
    form_layout->addRow(p->tsl("IP"), ip);
    form_layout->addRow(p->tsl("Username"), username);
    form_layout->addRow(p->tsl("Password"), password);

    connect(help_button, SIGNAL(clicked()), this, SLOT(help()));
    connect(model_search, SIGNAL(textEdited(const QString &)), this, SLOT(search_in_model()));

    layout->addWidget(reconnect_group_box);
    return page;
}


QString configure_dialog::get_var(const string &var, var_type typ){
        ddclient_gui *p = (ddclient_gui *) this->parent();
        downloadc *dclient = p->get_connection();

        string answer;

        if(!p->check_connection(true, "Please connect before configurating DownloadDaemon."))
                return QString("");

        QMutex *mx = p->get_mutex();
        mx->lock();

        try{
                if(typ == NORMAL_T) // get normal var
                        answer = dclient->get_var(var);
                else if(typ == ROUTER_T)  // get router var
                        answer = dclient->get_router_var(var);
                else // get premium var
                        answer = dclient->get_premium_var(var);
        }catch(client_exception &e){}

        mx->unlock();

        return QString(answer.c_str());
}


void configure_dialog::help(){
    ddclient_gui *p = (ddclient_gui *) this->parent();

    string help("HARD cancels all downloads if a reconnect is needed\n"
                "CONTINUE only cancels downloads that can be continued after the reconnect\n"
                "SOFT will wait until all other downloads are finished\n"
                "PUSSY will only reconnect if there is no other choice \n\t(no other download can be started without a reconnect)");

    QMessageBox::information(this, p->tsl("Help"), p->tsl(help));
}


void configure_dialog::search_in_model(){
    model->clear();
    QStringList model_list;

    string search_input = model_search->text().toStdString();

    for(size_t i=0; i<search_input.length(); i++) // lower everything for case insensitivity
        search_input[i] = tolower(search_input[i]);

    vector<string>::iterator it;
    for(it = router_model_list.begin(); it != router_model_list.end(); it++){
        string lower = *it;

        for(size_t i=0; i<lower.length(); i++) // lower everything for case insensitivity
            lower[i] = tolower(lower[i]);

        if(lower.find(search_input) != string::npos)
            model_list << it->c_str();
    }

    model->addItems(model_list);
}


void configure_dialog::ok(){
    QMessageBox::information(this, "^_____^", "I'm not doing anything....~");
    emit done(0);
}


void configure_dialog::save_premium(){
    QMessageBox::information(this, "^_____^", "I am premiumsaving....~");
}


void configure_dialog::save_password(){
    QMessageBox::information(this, "^_____^", "I am passwordsaving....~");
}
