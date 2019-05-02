#include "mespacket.h"

MESPacketBody::MESPacketBody(unsigned short key, QJsonObject &json)
{
    this->key=key;
    setData(json);
}

MESPacketBody::MESPacketBody(unsigned short key, QString &str)
{
    this->key=key;
    setData(str);
}

MESPacket::MESPacket()
{
    this->versions=0;
    this->length=0;
}

void MESPacket::setBody(MESPacketBody &mbody)
{
    body.key = mbody.key;
    body.length = mbody.length;
    body.data.prepend(mbody.data);
    length=mbody.data.length()+2+2;
}
