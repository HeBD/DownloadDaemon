HEADERS += ddclient_gui.h \
    ../include/downloadc/client_exception.h \
    ../include/downloadc/client_exception.h \
    ../include/downloadc/downloadc.h \
    ../include/netpptk/netpptk.h \
    ../include/crypt/md5.h \
    ../include/language/language.h \
    ddclient_gui_connect_dialog.h \
    ddclient_gui_update_thread.h \
    ddclient_gui_add_dialog.h \
    ddclient_gui_about_dialog.h \
    ddclient_gui_configure_dialog.h \
    ../include/cfgfile/cfgfile.h \
    ddclient_gui_status_bar.h
SOURCES += ddclient_gui.cpp \
    main.cpp \
    ../include/downloadc/downloadc.cpp \
    ../include/downloadc/client_exception.cpp \
    ../include/netpptk/netpptk.cpp \
    ../include/crypt/md5.cpp \
    ../include/language/language.cpp \
    ddclient_gui_connect_dialog.cpp \
    ddclient_gui_update_thread.cpp \
    ddclient_gui_add_dialog.cpp \
    ddclient_gui_about_dialog.cpp \
    ddclient_gui_configure_dialog.cpp \
    ../include/cfgfile/cfgfile.cpp \
    ddclient_gui_status_bar.cpp
INCLUDEPATH += ../include C:\Boost\include\boost-1_42
DESTDIR = ../../build/bin

win32 { 
    LIBS += -lws2_32
    RC_FILE = ddgui.rc
}

QMAKE_CXXFLAGS += -std=c++0x
