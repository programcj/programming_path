#include "wifidialog.h"
#include "ui_wifidialog.h"
#include <QStandardItemModel>

WIFIDialog::WIFIDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WIFIDialog)
{
    ui->setupUi(this);

    QStandardItemModel *standardItemModel = new QStandardItemModel(this);
    QStringList strList;
    strList.append("string1");
    strList.append("string2");
    strList.append("string3");
    strList.append("string4");
    strList.append("string5");
    strList.append("string6");
    strList.append("string7");
    strList << "string8";
    strList += "string9";
    int nCount = strList.size();
    for(int i = 0; i < nCount; i++)
    {
        QString string = static_cast<QString>(strList.at(i));
        QStandardItem *item = new QStandardItem(string);
        if(i % 2 == 1)
        {
            QLinearGradient linearGrad(QPointF(0, 0), QPointF(200, 200));
            linearGrad.setColorAt(0, Qt::darkGreen);
            linearGrad.setColorAt(1, Qt::yellow);
            QBrush brush(linearGrad);
            item->setBackground(brush);
        }
        standardItemModel->appendRow(item);
    }
     ui->listView->setModel(standardItemModel);
}

WIFIDialog::~WIFIDialog()
{
    delete ui;
}
