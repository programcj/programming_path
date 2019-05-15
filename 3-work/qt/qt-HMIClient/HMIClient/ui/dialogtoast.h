#ifndef DIALOGTOAST_H
#define DIALOGTOAST_H

#include <QDialog>
#include <QString>

namespace Ui {
class DialogToast;
}

class DialogToast : public QDialog
{
    Q_OBJECT

public:
    explicit DialogToast(QWidget *parent = 0);

     explicit DialogToast(QString message,QWidget *parent = 0);

    ~DialogToast();

    void setMessage(QString &message);

    void show(int interval);

    static void make(QString message,int toast);

private:
    virtual void timerEvent(QTimerEvent *event);

    Ui::DialogToast *ui;
};

#endif // DIALOGTOAST_H
