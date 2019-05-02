#ifndef MAINFORMBADPCS_H
#define MAINFORMBADPCS_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QSignalMapper>

namespace Ui {
class MainFormBadpcs;
}

class MainFormBadpcs : public QWidget
{
    int page;

    Q_OBJECT

public:
    explicit MainFormBadpcs(QWidget *parent = 0);
    ~MainFormBadpcs();

private slots:    
    void onMainDispatchOrderUpdate();

    void on_btPrevPage_clicked();

    void on_btNextPage_clicked();

    void on_btSaveBadNumber_clicked();

    void on_pushButton_Ok_clicked();

    void on_pushButton_No_clicked();

    void onInputClicked(QWidget *widget);

private:
    void onShowBadInfo(QString PCS_BadDataString);
    QSignalMapper *signalMapper;

    QLabel *uiGetBadName(int index);
    QLineEdit *uiGetBadValue(int index);
    QLabel *uiGetBadNewValue(int index);

    Ui::MainFormBadpcs *ui;
};

#endif // MAINFORMBADPCS_H
