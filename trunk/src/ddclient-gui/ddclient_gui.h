#ifndef DDCLIENT_GUI_H
#define DDCLIENT_GUI_H

#include <config.h>
#include <downloadc/downloadc.h>
#include <language/language.h>
#include <vector>

#include <QMainWindow>
#include <QString>
#include <QtGui/QMenu>
#include <QtGui/QLabel>
#include <QTreeView>
#include <QStandardItemModel>
#include <QItemSelectionModel>


struct selected_info{
    bool package;
    int row;
    int parent_row;
};


struct login_data{
        char host[256];
        int port;
        char pass[256];
        char lang[128];
};


class ddclient_gui : public QMainWindow{
    Q_OBJECT

    public:
	/** Defaultconstructor */
        ddclient_gui(QString config_dir);

	/** Destructor */
	~ddclient_gui();

        /** Updates Status of the Program, should be called after Connecting to a Server
        *    @param server Location of the Server, which the client is connected to
        */
        void update_status(QString server);

	/** Translates a string and changes it into a wxString
	*    @param text string to translate
	*    @returns translated wxString
	*/
	QString tsl(std::string text, ...);

        /** Setter for Language
        *    @param lang_to_set Language
        */
        void set_language(std::string lang_to_set);

        /** Getter for Download-Client Object (used for Communication with Daemon)
        *    @returns Download-Client Object
        */
        downloadc *get_connection();

    private:
        void add_bars();
        void update_bars();
        void add_list_components();
        void update_list_components();
        void get_selected_lines();

        QTreeView *list;
        QStandardItemModel *list_model;
        QItemSelectionModel *selection_model;
        QLabel *status_connection;

        QMenu *file_menu;
        QMenu *help_menu;
        QAction *connect_action;
        QAction *configure_action;
        QAction *activate_action;
        QAction *deactivate_action;
        QAction *activate_download_action;
        QAction *deactivate_download_action;
        QAction *add_action;
        QAction *delete_action;
        QAction *delete_finished_action;
        QAction *select_action;
        QAction *copy_action;
        QAction *quit_action;
        QAction *about_action;
        QAction *down_action;
        QAction *up_action;
        QToolBar *downloading_menu;

        QString config_dir;
        language lang;
        downloadc *dclient;
        std::vector<selected_info> selected_lines;

    private slots:
	void on_about();
	void on_select();
	void on_connect();
	void on_add();
	void on_delete();
	void on_delete_finished();
	void on_delete_file();
	void on_activate();
	void on_deactivate();
	void on_priority_up();
	void on_priority_down();
	void on_configure();
	void on_downloading_activate();
	void on_downloading_deactivate();
	void on_copy();
	void on_reload();

    protected:
        void contextMenuEvent(QContextMenuEvent *event);
        void resizeEvent(QResizeEvent* event);

};

#endif // DDCLIENT_GUI_H
