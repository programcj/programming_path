#ifndef MAINFORMDEVINFO_H
#define MAINFORMDEVINFO_H

#include <QWidget>

namespace Ui {
class MainFormDevInfo;
}

class MainFormDevInfo : public QWidget
{
    Q_OBJECT

public:
    explicit MainFormDevInfo(QWidget *parent = 0);
    ~MainFormDevInfo();

private slots:
    void on_pushButton_DO_0_clicked();

    void on_pushButton_DO_1_clicked();

    void on_pushButton_DO_2_clicked();

    void on_pushButton_DO_3_clicked();

private:
    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event);
    void uiShowGPIOStatus();
    Ui::MainFormDevInfo *ui;
};

#endif // MAINFORMDEVINFO_H
