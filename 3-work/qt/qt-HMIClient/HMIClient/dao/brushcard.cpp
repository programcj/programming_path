#include "brushcard.h"

BrushCard::BrushCard(QObject *parent) : QObject(parent)
{

}

BrushCard::~BrushCard()
{

}

QJsonObject BrushCard::toJsonObject()
{
    QJsonObject json;
    json.insert("CC_DispatchNo",CC_DispatchNo);
    json.insert("CC_CardID",CC_CardID);
    json.insert("CC_CardType", CC_CardType);
    json.insert("CC_CardData",CC_CardData);
    json.insert("CC_IsBeginEnd",CC_IsBeginEnd);
    json.insert("Name", Name);
    return json;
}

