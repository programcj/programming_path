#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QStackedWidget>
#include <QLabel>

#include "mainformfirst.h"
#include "mainformordermanagement.h"
#include "mainformbadpcs.h"
#include "mainformdevinfo.h"
#include "mainformprocessparameters.h"
#include "mainformsystemset.h"

namespace Ui {
class MainWindow;
}

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

    void on_bt_main_page6_clicked();

private:
    virtual void timerEvent(QTimerEvent *);

    void changleMainPage(int index);

    MainFormFirst *mainFormFirst;
    MainFormOrderManagement *mainFormOrderManagement;
    MainFormBadpcs *mainFormBadpcs;
    MainFormDevInfo *mainFormDevInfo;
    MainFormProcessParameters *mainFormProcessParameters;
    MainFormSystemSet *mainFormSystemSet;

    Ui::MainWindow *ui;

};

#endif // MAINWINDOW_H
