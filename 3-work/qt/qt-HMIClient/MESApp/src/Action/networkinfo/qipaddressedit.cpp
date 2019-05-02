#include "qipaddressedit.h"
#include "qipaddressedititem.h"

#include <QtGui>
#include <QStringList>

QIpAddressEdit::QIpAddressEdit(QWidget *parent)
    : QWidget(parent)
{
    readOnly = false;
    item1 = new QIpAddressEditItem(this);
    item2 = new QIpAddressEditItem(this);
    item3 = new QIpAddressEditItem(this);
    item4 = new QIpAddressEditItem(this);
    item1->setMaximumSize(50, 40);
    item2->setMaximumSize(50, 40);
    item3->setMaximumSize(50, 40);
    item4->setMaximumSize(50, 40);
    item1->setNextItem(item2);
    item2->setNextItem(item3);
    item3->setNextItem(item4);
    item4->setPreviousItem(item3);
    item3->setPreviousItem(item2);
    item2->setPreviousItem(item1);

    QHBoxLayout * mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(item1);
    mainLayout->addWidget(new QLabel(".", this));
    mainLayout->addWidget(item2);
    mainLayout->addWidget(new QLabel(".", this));
    mainLayout->addWidget(item3);
    mainLayout->addWidget(new QLabel(".", this));
    mainLayout->addWidget(item4);

    this->setLayout(mainLayout);
}

QIpAddressEdit::~QIpAddressEdit()
{
    delete item1;
    delete item2;
    delete item3;
    delete item4;
}

void QIpAddressEdit::setText(const QString &ip)
{
    QStringList itemsText = ip.split(".");
    if(itemsText.size() == 4)
    {
        item1->setText(itemsText[0]);
        item2->setText(itemsText[1]);
        item3->setText(itemsText[2]);
        item4->setText(itemsText[3]);

        emit textChanged(ip);
        emit textEdited(ip);
    }
}

QString QIpAddressEdit::text() const
{
    return QString("%1.%2.%3.%4")
        .arg(item1->text())
        .arg(item2->text())
        .arg(item3->text())
        .arg(item4->text());
}

void QIpAddressEdit::setStyleSheet(const QString &styleSheet)
{
    item1->setStyleSheet(styleSheet);
    item2->setStyleSheet(styleSheet);
    item3->setStyleSheet(styleSheet);
    item4->setStyleSheet(styleSheet);
}

void QIpAddressEdit::ipChanged(const QString &)
{
    emit textChanged(text());
}

void QIpAddressEdit::ipEdited(const QString &)
{
    emit textEdited(text());
}

void QIpAddressEdit::setReadOnly(bool model)
{
    readOnly = model;
    item1->setReadOnly(model);
    item2->setReadOnly(model);
    item3->setReadOnly(model);
    item4->setReadOnly(model);
}

void QIpAddressEdit::clear()
{
    item1->clear();
    item2->clear();
    item3->clear();
    item4->clear();
}
