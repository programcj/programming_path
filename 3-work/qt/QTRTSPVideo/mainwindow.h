#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QMap>
#include <QStringList>
#include <QListWidgetItem>

#include <jzlxsapplication.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QLabel *statusTimeLabel;
    QMap<QString,QString> mRTSPClientInfoMap;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    virtual void timerEvent(QTimerEvent *);

private slots:
    void rtspServerStart(const QString &urlPrefix);
    void rtspServerClose();

    void rtspClientConnect(const QString ip);
    void rtspClientClose(const QString ip);

    void rtspClientDescribe(const QString &clientIP,const QString &urlSuffix);

    void on_btSelectPath_clicked();

    void on_btRTSPRun_clicked();

    void on_tableRTSPInfo_customContextMenuRequested(const QPoint &pos);

    void on_listLocalIP_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
