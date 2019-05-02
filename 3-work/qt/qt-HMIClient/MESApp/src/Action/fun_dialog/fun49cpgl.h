#ifndef FUN48CPGL_H
#define FUN48CPGL_H

#include <QDialog>

namespace Ui {
class Fun49cpgl;
}

class Fun49cpgl : public QDialog
{
    Q_OBJECT
    
public:
    explicit Fun49cpgl(QWidget *parent = 0);
    ~Fun49cpgl();
    
private:
    Ui::Fun49cpgl *ui;
};

#endif // FUN48CPGL_H
