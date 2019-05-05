#-------------------------------------------------
#
# Project created by QtCreator 2017-03-07T22:48:51
#
#-------------------------------------------------

QT       +=  core \
    gui \
    sql \
    xml \
    network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = hmiclient
TEMPLATE = app

SOURCES += main.cpp \
    ui/mainformbadpcs.cpp \
    ui/mainformdevinfo.cpp \
    ui/mainformfirst.cpp \
    ui/mainformordermanagement.cpp \
    ui/mainformprocessparameters.cpp \
    ui/mainformsystemset.cpp \
    ui/mainwindow.cpp \
    tools/syslog.cpp \
    dao/sqliteopenhelper.cpp \
    dao/sqlitebasehelper.cpp \
    application.cpp \
    netinterface/mespacket.cpp \
    netinterface/mesmqttclient.cpp \
    netinterface/mesmsghandler.cpp \
    tools/mestools.cpp \
    dao/mesdispatchorder.cpp \
    hal/servicedicycle.cpp \
    ui/dialogswingcard.cpp \
    hal/halrfidread.cpp \
    appconfig.cpp \
    hal/am335x_interface/led_test.c \
    dao/mesproducteds.cpp \
    ui/dialogtoast.cpp \
    ui/dialoginput.cpp \
    hal/servicemodulekang.cpp \
    dao/brushcard.cpp \
    ui/dialognetseteth0.cpp \
    ui/dialognetsetwlan.cpp \
    hal/am335x_interface/gpio.c \
    netinterface/netbroadhandler.cpp

HEADERS  += ui/mainwindow.h \
    ui/mainformbadpcs.h \
    ui/mainformdevinfo.h \
    ui/mainformfirst.h \
    ui/mainformordermanagement.h \
    ui/mainformprocessparameters.h \
    ui/mainformsystemset.h \
    tools/syslog.h \
    tools/MQTTClient.h \
    tools/MQTTClientPersistence.h \
    dao/sqliteopenhelper.h \
    dao/sqlitebasehelper.h \
    application.h \
    netinterface/mespacket.h \
    netinterface/mesmqttclient.h \
    netinterface/mesmsghandler.h \
    dao/pushmsg.h \
    tools/mestools.h \
    dao/mesdispatchorder.h \
    hal/servicedicycle.h \
    ui/dialogswingcard.h \
    hal/halrfidread.h \
    appconfig.h \
    hal/am335x_interface/interface.h \
    dao/mesproducteds.h \
    ui/dialogtoast.h \
    public.h \
    ui/dialoginput.h \
    hal/servicemodulekang.h \
    dao/brushcard.h \
    ui/dialognetseteth0.h \
    ui/dialognetsetwlan.h \
    netinterface/netbroadhandler.h


FORMS    += testdialog.ui \
    ui/mainformbadpcs.ui \
    ui/mainformdevinfo.ui \
    ui/mainformfirst.ui \
    ui/mainformordermanagement.ui \
    ui/mainformprocessparameters.ui \
    ui/mainformsystemset.ui \
    ui/mainwindow.ui \
    ui/dialogswingcard.ui \
    ui/dialogtoast.ui \
    ui/dialoginput.ui \
    ui/dialognetseteth0.ui \
    ui/dialognetsetwlan.ui

RESOURCES += \
    app.qrc

LIBS += -lpaho-mqtt3c
#unix:LIBS += -L$$PWD/lib/am335x -lpaho-mqtt3c
#unix:LIBS += -L$$PWD/lib/weiqian -lpaho-mqtt3c
#macx:LIBS += -L$$PWD/lib/macx/ -lpaho-mqtt3c -lpthread

target.path=/home/root/
INSTALLS +=target

#export PATH=/opt/yogurt/AM335x-PD15.2.1/sysroots/x86_64-yogurtsdk-linux/usr/bin/arm-phytec-linux-gnueabi/:$PATH
#/opt/yogurt/AM335x-PD15.2.1/sysroots/x86_64-yogurtsdk-linux/usr/bin/arm-phytec-linux-gnueabi/

QMAKE_CXXFLAGS += -Wno-unused-parameter

QMAKE_CXXFLAGS += -Wno-unused-variable

DISTFILES += \
    readme.txt
