#include "productionmodeitem.h"
#include "ui_productionmodeitem.h"

ProductionModeItem::ProductionModeItem(QWidget *parent) :
        QWidget(parent), ui(new Ui::ProductionModeItem)
{
    ui->setupUi(this);
}

ProductionModeItem::~ProductionModeItem()
{
    delete ui;
}

void ProductionModeItem::setMode(int v)
{
    QString str;
    str.sprintf("%d模", v);
    ui->labelMode->setText(str);
}

void ProductionModeItem::setStartTime(const QString& str)
{
    //ui->labelStartTime->setText(str);
}

void ProductionModeItem::setEndTime(const QString& str)
{
    ui->labelEndTime->setText(str);
}

/**
 * 模次,开始时间,结束时间, 机器周期, 填充周期,成型周期
 */
void ProductionModeItem::setContent(int number, const QString& startTime,
        const QString& endTime,
        unsigned long machineCycle,
        unsigned long FillTime,
        unsigned long CycleTime)
{
    setMode(number);
    setEndTime(endTime.mid(11,-1));
    //机器周期不显示
    QString tmp;
    tmp.sprintf("%lds",FillTime);
    ui->label_4->setText(tmp);
    tmp.sprintf("%lds",CycleTime);
    ui->label_3->setText(tmp);
}
