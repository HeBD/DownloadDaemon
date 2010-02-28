#ifndef DDCLIENT_GUI_H
#define DDCLIENT_GUI_H

#include <QMainWindow>
#include <QString>
#include <QtGui/QMenu>

class ddclient_gui : public QMainWindow{
    Q_OBJECT

    public:    
	/** Defaultconstructor */
	ddclient_gui();

	/** Destructor */
	~ddclient_gui();

	/** Translates a string and changes it into a wxString
	*    @param text string to translate
	*    @returns translated wxString
	*/
	QString tsl(std::string text, ...);

    private:
	void add_bars();




	QMenu* file_menu;
	QAction* activate_action;
	QAction* deactivate_action;

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
	void on_resize();
	void on_reload();
	void on_right_click();

};

#endif // DDCLIENT_GUI_H
