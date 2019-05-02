#ifndef QIPADDRESSEDITITEM_H
#define QIPADDRESSEDITITEM_H

#include <QString>
#include <QLineEdit>

class QWidget;
class QFocusEvent;
class QKeyEvent;

class QIpAddressEditItem : public QLineEdit
{
    Q_OBJECT
public:
    explicit QIpAddressEditItem(QWidget *parent = 0);

    void setNextItem(QLineEdit *lineEdit) { nextItem = lineEdit; }
    void setPreviousItem(QLineEdit *lineEdit) { previousItem = lineEdit; }
    
protected:
    virtual void focusInEvent(QFocusEvent *);
    virtual void keyPressEvent(QKeyEvent *);
signals:
    
public slots:
    void itemEdited(const QString &);

private:
    QLineEdit *nextItem;
    QLineEdit *previousItem;
    
};

#endif // QIPADDRESSEDITITEM_H
