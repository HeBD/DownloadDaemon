#include "ddclient_gui_about_dialog.h"
#include "ddclient_gui.h"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QDialogButtonBox>
#include <QPixmap>
#include <QFont>

using namespace std;

#ifndef DDCLIENT_GUI_VERSION
        #define DDCLIENT_GUI_VERSION "unknown"
#endif


about_dialog::about_dialog(QWidget *parent, QString build) : QDialog(parent){
    ddclient_gui *p = (ddclient_gui *) parent;

    setWindowTitle(p->tsl("About..."));

    QHBoxLayout *layout = new QHBoxLayout();
    QVBoxLayout *text_layout = new QVBoxLayout();
    QDialogButtonBox *button_box = new QDialogButtonBox(QDialogButtonBox::Ok);

    QPixmap logo("img/logoDD.png");
    QLabel *logo_label = new QLabel();
    logo_label->setPixmap(logo);

    layout->addWidget(logo_label);
    layout->addLayout(text_layout);
    this->setLayout(layout);

    button_box->button(QDialogButtonBox::Ok)->setDefault(true);
    button_box->button(QDialogButtonBox::Ok)->setFocus(Qt::OtherFocusReason);

    QString name("<h3>DownloadDaemon Client GUI</h3>");

    QString version;
    version += "Version: ";
    version += DDCLIENT_GUI_VERSION;

    QLabel *url = new QLabel("<a href='http://downloaddaemon.sourceforge.net/'>Website</a>");
    url->setOpenExternalLinks(true);

    QString copyright = "\nCopyright 2009-2010 by the DownloadDaemon team.\nAll rights reserved.\n\n";
    copyright += "The program is provided AS IS with NO WARRANTY\nOF ANY KIND, INCLUDING THE WARRANTY ";
    copyright += "OF DESIGN,\nMERCHANTABILITY AND FITNESS FOR A PARTICULAR\nPURPOSE.";

    text_layout->addWidget(new QLabel(name));
    text_layout->addWidget(new QLabel(version));
    text_layout->addWidget(new QLabel(build));
    text_layout->addWidget(url);
    text_layout->addWidget(new QLabel(copyright));
    text_layout->addWidget(button_box, 0, Qt::AlignRight);

    connect(button_box->button(QDialogButtonBox::Ok), SIGNAL(clicked()),this, SLOT(reject()));
}
