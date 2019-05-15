#ifndef MAINFORMORDERMANAGEMENT_H
#define MAINFORMORDERMANAGEMENT_H

#include <QWidget>
#include <QSignalMapper>
#include <QPushButton>
#include "../application.h"

namespace Ui {
class MainFormOrderManagement;
}

class MainFormOrderManagement : public QWidget
{
    Q_OBJECT

public:
    explicit MainFormOrderManagement(QWidget *parent = 0);
    ~MainFormOrderManagement();

private slots:
    void doClicked(QWidget *widget);

    void onMainDispatchOrderUpdate();

    void on_pushButton_PrevItem_clicked();

    void on_pushButton_NextItem_clicked();

    void on_pushButton_report_clicked();

private:
    QSignalMapper *signalMapper;

    int orderItemIndex;

    QJsonObject configInfoLoad(bool defaultConfig=false);

    void configInfoSave(QJsonObject &jsonConfig);

    void onButtonClicked(QPushButton *button);

    void onShowAllFunButton();

    void onShowMainDispatchOrder();
    Ui::MainFormOrderManagement *ui;
};

#endif // MAINFORMORDERMANAGEMENT_H
