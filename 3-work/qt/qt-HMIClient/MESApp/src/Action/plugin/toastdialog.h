#ifndef TOASTDIALOG_H
#define TOASTDIALOG_H

#include <QDialog>
#include <QtGui>

namespace Ui {
class ToastDialog;
}

class ToastDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ToastDialog(QWidget *parent = 0);
    explicit ToastDialog(const QString &text,int timems,QWidget *parent=0);
    ~ToastDialog();
    
    static void Toast(QWidget *parent,const QString &text,int timems);

private:
    QPropertyAnimation *animation;
    virtual void timerEvent(QTimerEvent *e);
    int m_times;
    Ui::ToastDialog *ui;
};

#endif // TOASTDIALOG_H
