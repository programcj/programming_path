/********************************************
 * 温度 压力 大小显示，GraphicsView中.
 * 改变 Value 就可以了
 *
 *******************************************/
#ifndef TEMPERATUREITEM_H
#define TEMPERATUREITEM_H

#include <QGraphicsItem>

class TemperatureItem: public QGraphicsItem {
	QPixmap pixMap;
	QPixmap pixMap_blud;
	QPixmap pixMap_gren;

	QString title;
	int Value;
	QSize size;
public:
	int getValue();
	void setValue(int v);

	TemperatureItem();

	const QSize &Size();

	void setTitle(const QString &title);
	virtual QRectF boundingRect() const;

	virtual void paint(QPainter *painter,
			const QStyleOptionGraphicsItem *option, QWidget *widget);
};

#endif // TEMPERATUREITEM_H
