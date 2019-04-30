#-------------------------------------------------
#
# Project created by QtCreator 2019-03-25T10:15:43
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = JZLXS_VideoTools
TEMPLATE = app
QMAKE_CXXFLAGS += -stdlib=libc++

SOURCES += main.cpp\
        mainwindow.cpp \
    qrtspserver.cpp \
    jzlxsapplication.cpp \
    qlivertspserver.cpp \
    qmediafileinfo.cpp \
    LiveEx/qbytestreamfilesourceloop.cpp \
    LiveEx/qh264videofileservermediasubsessionloop.cpp \
    LiveEx/qlivemp4bytestreamfilesource.cpp \
    LiveEx/qlivemp4fileservermediasubsession.cpp \
    qsysteminfo.cpp

HEADERS  += mainwindow.h \
    qrtspserver.h \
    jzlxsapplication.h \
    qlivertspserver.h \
    qmediafileinfo.h \
    LiveEx/qbytestreamfilesourceloop.h \
    LiveEx/qh264videofileservermediasubsessionloop.h \
    LiveEx/qlivemp4bytestreamfilesource.h \
    LiveEx/qlivemp4fileservermediasubsession.h \
    qsysteminfo.h

FORMS    += mainwindow.ui

LIVE555 = $$PWD/../live555

CONFIG(release, debug|release){
    LIVE555_LIBS= $$PWD/../build/live555/

    LIBS += $$LIVE555_LIBS/liveMedia/release/libliveMedia.a
    LIBS += $$LIVE555_LIBS/groupsock/release/libgroupsock.a
    LIBS += $$LIVE555_LIBS/BasicUsageEnvironment/release/libBasicUsageEnvironment.a
    LIBS += $$LIVE555_LIBS/UsageEnvironment/release/libUsageEnvironment.a
}

CONFIG(debug, debug|release){
    LIVE555_LIBS= $$PWD/../build/live555/

    LIBS += $$LIVE555_LIBS/liveMedia/debug/libliveMedia.a
    LIBS += $$LIVE555_LIBS/BasicUsageEnvironment/debug/libBasicUsageEnvironment.a
    LIBS += $$LIVE555_LIBS/UsageEnvironment/debug/libUsageEnvironment.a
    LIBS += $$LIVE555_LIBS/groupsock/debug/libgroupsock.a
}

LIBS += -lWs2_32

INCLUDEPATH += $$LIVE555/UsageEnvironment/include \
        $$LIVE555/groupsock/include \
        $$LIVE555/liveMedia/include \
        $$LIVE555/BasicUsageEnvironment/include

FFMPEG_LIBSPATH = $$PWD/../ffmpeg-3.2.4-win32-dev

INCLUDEPATH += $$FFMPEG_LIBSPATH/include
LIBS += -L$$FFMPEG_LIBSPATH/lib -lavutil -lavformat -lavcodec


