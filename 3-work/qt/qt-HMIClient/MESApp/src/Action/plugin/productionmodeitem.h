#ifndef PRODUCTIONMODEITEM_H
#define PRODUCTIONMODEITEM_H

#include <QWidget>

namespace Ui
{
class ProductionModeItem;
}

class ProductionModeItem: public QWidget
{
Q_OBJECT

public:
    explicit ProductionModeItem(QWidget *parent = 0);
    ~ProductionModeItem();

    void setMode(int v);
    void setStartTime(const QString &str);
    void setEndTime(const QString &str);

    void setContent(int number, const QString &startTime, const QString &endTime,
            unsigned long machineCycle, unsigned long FillTime,
            unsigned long CycleTime);
private:
    Ui::ProductionModeItem *ui;
};

#endif // PRODUCTIONMODEITEM_H
