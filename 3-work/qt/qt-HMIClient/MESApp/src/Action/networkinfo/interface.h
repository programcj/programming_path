#ifndef INTERFACE_H
#define INTERFACE_H
#include <QString>

class Interface
{
public:
    Interface(QString &name);
public:
    QString name;
   // bool    dhcp;
    QString ip;
    QString mask;
    QString gateway;
   // QString dns;
    QString serverip;
    unsigned int portA;
    unsigned int portB;

};

#endif // INTERFACE_H
