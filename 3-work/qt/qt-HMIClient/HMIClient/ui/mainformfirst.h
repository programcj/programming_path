#ifndef MAINFORMFIRST_H
#define MAINFORMFIRST_H

#include <QWidget>

namespace Ui {
class MainFormFirst;
}

class MainFormFirst : public QWidget
{
    Q_OBJECT

public:
    explicit MainFormFirst(QWidget *parent = 0);
    ~MainFormFirst();

private slots:
    void onMainDispatchOrderUpdate();

    void on_btItemUp_clicked();

    void on_btItemNext_clicked();

    void on_btItemUpItem_clicked();

    void on_btItemNextItem_clicked();

private:
    int orderIndex;
    int orderItemIndex;

    void onShowOrder(int index);
    void onShowOrderItem(QString MO_DispatchNo,int index);
    Ui::MainFormFirst *ui;
};

#endif // MAINFORMFIRST_H
