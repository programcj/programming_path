#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QStackedWidget>
#include <QLabel>

#include "../Public/public.h"
#include "../HAL/backlight/Backlight.h"

#include "mainhomepage.h"
#include "mainproductorderpage.h"
#include "mainqccheckpage.h"
#include "mainprocedureparampage.h"
#include "mainconfigpage.h"
#include "plugin/mesnoticedialog.h"

namespace Ui {
class MainWindow;
}
/**
 * 主界面
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_bt_main_page1_clicked();

    void on_bt_main_page2_clicked();

    void on_bt_main_page3_clicked();

    void on_bt_main_page4_clicked();

    void on_bt_main_page5_clicked();

    void CSCPConnectStatus(bool flag,const QString &mess); //连接是否成功

    void OnMESMailMess(QString startT, QString endT, QString mess);

    void on_btMenu_clicked();

    void on_btUserInfo_clicked();

    void on_SysMess_clicked();

private:
    void on_update_UI();
    virtual void timerEvent(QTimerEvent *);

    QStringList noteList;
    MainHomePage *page1;
    MainProductOrderPage *page2;
    MainQCCheckPage *page3;
    MainProcedureParamPage *page4;
    MainConfigPage *page5;

    QPropertyAnimation *animation;

    Ui::MainWindow *ui;
    QLabel *msgLabel;  //状态栏信息
};

#endif // MAINWINDOW_H
