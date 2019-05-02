#ifndef MAINCONFIGPAGE_H
#define MAINCONFIGPAGE_H

#include <QWidget>
#include <QProcess>

namespace Ui {
class MainConfigPage;
}

class fileinfo
{
public:
	QString filename;
	QString filesize;
	QString filever;
	QString filestate;
};
class MainConfigPage : public QWidget
{
    Q_OBJECT
    
public:
    explicit MainConfigPage(QWidget *parent = 0);
    ~MainConfigPage();
    QProcess *proc;
    void init_deviceInfo();
    void init_sysInfo();
    void init_switchInfo();
    void initNetMap(QString name);
    void sysInfoMapFun();
    void init_button_state();
    void switchInfoMapFun();
public: //index 2
    QList<fileinfo> dirfile;
    QStringList fileList;
    QString dirpath;
    void showfile();
    void getFileInfo();
    static void setFileList(QList<QString> &name);
    void dirfilelist();


 signals:
     void exitprocess();

private slots:
    void on_btDevID_clicked();
    void on_btMachineID_clicked();
    void on_btNetconfig_clicked();
    void on_btCardPass_clicked();
    void on_btSystemTime_clicked();
    void on_btTempAdjust_clicked();
    void on_btNetTest_clicked();
    void on_btWifi_clicked();
    void on_btExit_clicked();
    void on_btRestart_clicked();
    void on_btReboot_clicked();
    void on_bt_calib_clicked();

    void on_bt_voice_clicked();
    void on_bt_usb_clicked();
    void handleTabPress(int index);

    void on_bt_NetCommand_clicked();

private:
    Ui::MainConfigPage *ui;
};

#endif // MAINCONFIGPAGE_H
