#ifndef INPUTTIME_H
#define INPUTTIME_H

#include <QtGui/QDialog>
#include "ui_inputtime.h"

class InputTime : public QDialog
{
    Q_OBJECT

public:
    InputTime(QWidget *parent = 0);
    ~InputTime();
    QString getEditTimeText();

private:
    Ui::InputTimeClass ui;
};

#endif // INPUTTIME_H
