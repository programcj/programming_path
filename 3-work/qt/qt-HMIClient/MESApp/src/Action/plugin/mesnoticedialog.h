#ifndef MESNOTICEDIALOG_H
#define MESNOTICEDIALOG_H

#include <QMouseEvent>
#include <QDialog>

namespace Ui {
class MESNoticeDialog;
}

class MESNoticeDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit MESNoticeDialog(QWidget *parent = 0);
    ~MESNoticeDialog();
    
    void setText(const QString &str);

private slots:
    void on_ptOK_clicked();

private:
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void focusOutEvent(QFocusEvent *e);
    Ui::MESNoticeDialog *ui;
};

#endif // MESNOTICEDIALOG_H
