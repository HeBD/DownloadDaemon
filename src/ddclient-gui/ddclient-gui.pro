HEADERS += ddclient_gui.h \
    ../include/downloadc/client_exception.h \
    ../include/downloadc/client_exception.h \
    ../include/downloadc/downloadc.h \
    ../include/netpptk/netpptk.h \
    ../include/crypt/md5.h
SOURCES += ddclient_gui.cpp \
    main.cpp \
    ../include/downloadc/downloadc.cpp \
    ../include/downloadc/client_exception.cpp \
    ../include/netpptk/netpptk.cpp \
    ../include/crypt/md5.cpp
INCLUDEPATH += ../include
LIBS += -lboost_thread-mt
DESTDIR = ../../build/bin
