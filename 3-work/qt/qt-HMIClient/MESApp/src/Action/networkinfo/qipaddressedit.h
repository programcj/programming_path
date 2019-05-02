#ifndef QIPADDRESSEDIT_H
#define QIPADDRESSEDIT_H

#include <QWidget>

class QIpAddressEditItem;

class QIpAddressEdit : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(bool readOnly READ isReadOnly() WRITE setReadOnly)
    
public:
    QIpAddressEdit(QWidget *parent = 0);
    ~QIpAddressEdit();

    QString text() const;
    void setStyleSheet(const QString &styleSheet);
    void setReadOnly(bool);
    bool isReadOnly() const { return readOnly; }

public slots:
    void setText(const QString &);
    void clear();

private slots:
    void ipChanged(const QString &);
    void ipEdited(const QString &);

signals:
    void textChanged(const QString &);
    void textEdited(const QString &);

private:
    QIpAddressEditItem *item1;
    QIpAddressEditItem *item2;
    QIpAddressEditItem *item3;
    QIpAddressEditItem *item4;

    bool readOnly;
};

#endif // QIPADDRESSEDIT_H
