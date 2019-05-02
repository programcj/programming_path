#ifndef MAINFORMPROCESSPARAMETERS_H
#define MAINFORMPROCESSPARAMETERS_H

#include <QWidget>

namespace Ui {
class MainFormProcessParameters;
}

class MainFormProcessParameters : public QWidget
{
    Q_OBJECT

public:
    explicit MainFormProcessParameters(QWidget *parent = 0);
    ~MainFormProcessParameters();

public Q_SLOTS:
    void onTotalNumAdd(int TotalNum,int endmsc);

private:
    virtual void timerEvent(QTimerEvent *);

    Ui::MainFormProcessParameters *ui;
};

#endif // MAINFORMPROCESSPARAMETERS_H
