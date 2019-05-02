#ifndef BRUSHCARD_H
#define BRUSHCARD_H

#include <QObject>
#include "../public.h"

class BrushCard : public QObject
{
    Q_OBJECT
public:
    explicit BrushCard(QObject *parent = 0);
    ~BrushCard();

    QString CC_DispatchNo;
    QString CC_CardID;
    int CC_CardType;
    QString CC_CardData;
    int CC_IsBeginEnd;
    QString Name;

    QJsonObject toJsonObject();

signals:
public slots:
};

#endif // BRUSHCARD_H
