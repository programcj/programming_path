#include <QtGui>
#include <QDebug>

#include "temperatureitem.h"

TemperatureItem::TemperatureItem() {
    Value = 0;
    pixMap_gren.load(":/rc/res/temperature_gren.bmp");
    pixMap_blud.load(":/rc/res/temperature_blue.bmp");
    pixMap.load(":/rc/res/temperature.png");

    size.setHeight(pixMap.height() + 10 + 40);
    size.setWidth(pixMap.width() + 20);
}

QRectF TemperatureItem::boundingRect() const {
    return QRectF(0, 0, size.width(), size.height());
}

void TemperatureItem::setTitle(const QString& title) {
    this->title = title;
}

const QSize &TemperatureItem::Size() {
    return size;
}

void TemperatureItem::paint(QPainter *painter,
        const QStyleOptionGraphicsItem *option, QWidget *widget) {
    int h;
    h=Value;
    if(Value>450) {
        h=455;
    }
    //qDebug()<< "TemperatureItem update"   ;
    h = h * (1.0 * pixMap_blud.height() / 450);

    painter->drawPixmap(0, 0, pixMap.width(), pixMap.height(), pixMap);
    painter->drawPixmap(25, 15, pixMap_gren.width(), pixMap_gren.height(),
            pixMap_gren);

    QPixmap wn = pixMap_blud.scaled(pixMap_blud.width(), h);

    painter->drawPixmap(33, 15 + pixMap_blud.height() - h, wn.width(),
            wn.height(), wn); //绘制蓝色高度

    QPen pen;
    pen.setColor(Qt::white);
    painter->setPen(pen);
    painter->drawText(10, pixMap.height() + 10, title);

    pen.setColor(Qt::white);
    painter->setPen(pen);
    painter->drawText(0, pixMap.height() + 10 + 20, "标准:300℃");

    QString str;
    str.sprintf("当前:%d℃", Value);

    pen.setColor(Qt::white);
    painter->setPen(pen);
    painter->drawText(0, pixMap.height() + 10 + 40, str);
}

int TemperatureItem::getValue() {
    return Value;
}

void TemperatureItem::setValue(int v) {
    this->Value = v;
    this->update(boundingRect());
}
