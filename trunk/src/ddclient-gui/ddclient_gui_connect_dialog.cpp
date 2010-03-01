#include "ddclient_gui_connect_dialog.h"
#include "ddclient_gui.h"
#include <QtGui/QGroupBox>
#include <QtGui/QFormLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QDialogButtonBox>

connect_dialog::connect_dialog(QWidget *parent) : QDialog(parent){
    ddclient_gui *p = (ddclient_gui *) parent;

    setWindowTitle(p->tsl("Connect to Host"));

    QGroupBox *connect_box = new QGroupBox(p->tsl("Data"));
    QFormLayout *form_layout = new QFormLayout();
    //QHBoxLayout *button_layout = new QHBoxLayout();
    QVBoxLayout *layout = new QVBoxLayout();
    QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect_box->setLayout(form_layout);
    layout->addWidget(new QLabel(p->tsl("Please insert Host Data")));
    layout->addWidget(connect_box);
    layout->addWidget(button_box);
    this->setLayout(layout);

    host = new QLineEdit("127.0.0.1");
    port = new QLineEdit("56789");
    port->setMaxLength(5);
    port->setFixedWidth(50);
    password = new QLineEdit();
    password->setEchoMode(QLineEdit::Password);
    language = new QComboBox();
    language->addItem("English");
    language->addItem("Deutsch");
    save_data = new QCheckBox();

    form_layout->addRow(new QLabel(p->tsl("IP/URL")), host);
    form_layout->addRow(new QLabel(p->tsl("Port")), port);
    form_layout->addRow(new QLabel(p->tsl("Password")), password);
    form_layout->addRow(new QLabel(p->tsl("Language")), language);
    form_layout->addRow(new QLabel(p->tsl("Save Data")), save_data);

    connect(button_box->button(QDialogButtonBox::Ok), SIGNAL(clicked()),this, SLOT(ok()));
    connect(button_box->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),this, SLOT(reject()));
}

void connect_dialog::ok(){}

