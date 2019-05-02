#ifndef TEMPECALIBRATE_H
#define TEMPECALIBRATE_H

#include <QtGui/QDialog>
#include "ui_tempecalibrate.h"

class tempecalibrate : public QDialog
{
    Q_OBJECT

public:
    tempecalibrate(QWidget *parent = 0);
    ~tempecalibrate();
    QString getEditTempValue();
    int getCombChannelValue();

private:
    Ui::tempecalibrateClass ui;
};

#endif // TEMPECALIBRATE_H
