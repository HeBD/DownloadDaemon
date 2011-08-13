/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_GUI_H
#define DDCLIENT_GUI_H

#include <config.h>
#include <downloadc/downloadc.h>
#include <language/language.h>
#include <vector>
#include <QThread>

#include <QMainWindow>
#include <QString>
#include <QtGui/QMenu>
#include <QtGui/QLabel>
#include <QTreeView>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QMutex>
#include <QClipboard>
#include <QSystemTrayIcon>


struct selected_info{
    bool package;
    int row;
    int parent_row;
};

struct view_info{
    bool package;
    int id;
    int package_id;
    bool expanded;
    bool selected;
};

struct login_data{
    std::vector<std::string> host;
    std::vector<int> port;
    std::vector<std::string> pass;
    int selected;
    std::string lang;
};

enum error_message{error_none, error_selected, error_connected};


class ddclient_gui : public QMainWindow{
    Q_OBJECT

    public:
        /** Constructor
        *   @param config_dir Configuration Directory of Program
        */
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

        /** Getter for Mutex, that makes sure only one Thread uses Downloadc (connection to Daemon) and Content List at a time
        *    @returns Mutex
        */
        QMutex *get_mutex();

        /** Setter for Language
        *    @param lang_to_set Language
        */
        void set_language(std::string lang_to_set);

        /** Setter for Update Interval
        *    @param interval Interval to set
        */
        void set_update_interval(int interval);

        /** Getter for Download-Client Object (used for Communication with Daemon)
        *    @returns Download-Client Object
        */
        downloadc *get_connection();

        /** Gets the download list from Daemon and emits do_reload() signal
        *   @param update only get updates, not the whole list (subscriptions)
        */
        void get_content(bool update = false);

        /** Checks connection
        *    @param tell_user show to user via message box if connection is lost
        *    @param individual_message part of a message to send to user
        *    @returns connection available or not
        */
        bool check_connection(bool tell_user = false, std::string individual_message = "");

        /** Checks if there is subscription enabled and subscribes to SUBS_DOWNLOADS, if so
        *   @return enabled or not
        */
        bool check_subscritpion();

        /** Clears the error message cache, so that they will be shown again */
        void clear_last_error_message();

        /** Calculates the packages progress by comparing the number of all package downloads and finished package downloads
        *   @param package_row row where the package is stored in the view
        *   @returns package progress in %
        */
        int calc_package_progress(int package_row);

    signals:
        void do_reload();
        void captcha_error();

    private:
        void add_bars();
        void update_bars();
        void add_list_components();
        void update_list_components();
        void add_tray_icon();
        void cut_time(std::string &time_left);
        std::string build_status(std::string &status_text, std::string &time_left, download &dl);
        bool check_selected();
        void deselect_lines();
        void get_selected_lines();
        static bool sort_selected_info(selected_info i1, selected_info i2);
        std::vector<view_info> get_current_view();
        void update_packages();
        void compare_packages();
        void compare_downloads(QModelIndex &index, std::vector<package>::iterator &new_it, std::vector<package>::iterator &old_it, std::vector<view_info> &info);

        QMutex mx;
        downloadc *dclient;
        language lang;
        QString server;
        uint64_t not_downloaded_yet;
        uint64_t selected_downloads_size;
        int selected_downloads_count;
        double download_speed;
        error_message last_error_message;
        std::vector<selected_info> selected_lines;
        std::vector<package> content;
        std::vector<package> new_content;
        std::vector<update_content> new_updates;
        QString config_dir;
        QThread *thread;
        bool full_list_update, reload_list;

        QTreeView *list;
        QStandardItemModel *list_model;
        QItemSelectionModel *selection_model;
        QLabel *status_connection;

        QMenu *file_menu;
        QMenu *help_menu;
        QMenu *download_menu;
        QAction *connect_action;
        QAction *configure_action;
        QAction *activate_action;
        QAction *deactivate_action;
        QAction *activate_download_action;
        QAction *deactivate_download_action;
        QAction *container_action;
        QAction *add_action;
        QAction *delete_action;
        QAction *delete_finished_action;
        QAction *select_action;
        QAction *copy_action;
        QAction *paste_action;
        QAction *quit_action;
        QAction *about_action;
        QAction *down_action;
        QAction *up_action;
        QAction *top_action;
        QAction *bottom_action;
		QAction *captcha_action;
		QToolBar *configure_menu;
        QMenu *tray_menu;
        QSystemTrayIcon *tray_icon;

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
        void on_priority_top();
        void on_priority_bottom();
		void on_enter_captcha();
        void on_configure();
        void on_downloading_activate();
        void on_downloading_deactivate();
        void on_copy();
        void on_paste();
        void on_set_password();
        void on_set_name();
        void on_set_url();
        void on_load_container();
        void on_activate_tray_icon(QSystemTrayIcon::ActivationReason reason);
        void on_reload();
        void donate_flattr();
        void donate_sf();

    protected:
        void contextMenuEvent(QContextMenuEvent *event);
        void resizeEvent(QResizeEvent *event);
};

#endif // DDCLIENT_GUI_H
