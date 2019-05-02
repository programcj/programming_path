#include "qipaddressedititem.h"
#include <QtGui>
#include <QIntValidator>

QIpAddressEditItem::QIpAddressEditItem(QWidget *parent) :
    QLineEdit(parent)
{
    previousItem = NULL;
    nextItem = NULL;
    this->setMaxLength(3);
    this->setFrame(false);
    this->setAlignment(Qt::AlignCenter);

    QIntValidator *validator = new QIntValidator(this);
    validator->setBottom(0);
    this->setValidator(validator);

    connect(this, SIGNAL(textEdited(const QString &)),
            this, SLOT(itemEdited(const QString &)));
}

void QIpAddressEditItem::focusInEvent(QFocusEvent *e)
{
    this->selectAll();
    QLineEdit::focusInEvent(e);
}

void QIpAddressEditItem::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Period)
    {
        if(nextItem != NULL)
        {
            nextItem->setFocus();
            nextItem->selectAll();
        }
    }
    if(event->key() == Qt::Key_Backspace)
    {
        QString text = this->text();
        if(text.isEmpty() && previousItem != NULL)
        {
            previousItem->setFocus();
            previousItem->setCursorPosition(previousItem->cursorPosition());
            previousItem->backspace();
        }
    }
    if(event->key() == Qt::Key_Left)
    {
        if(this->cursorPosition() == 0 && previousItem != NULL)
        {
            previousItem->setFocus();
            previousItem->setCursorPosition(previousItem->cursorPosition());
        }
    }
    if(event->key() == Qt::Key_Right)
    {
        int pos = this->cursorPosition();
        int length = this->text().length();
        if(pos == length && nextItem != NULL)
        {
            nextItem->setFocus();
            nextItem->setCursorPosition(0);
        }
    }
    QLineEdit::keyPressEvent(event);
}

void QIpAddressEditItem::itemEdited(const QString &text)
{
    int num = text.toInt();
    if(num > 255 || num < 0)
    {
        QString message = text + " is not acceptable value.\n" +
                "Please input the value bewteen 0 with 255.";
        QMessageBox::warning(this,
                             tr("Warning"),
                             message);
        this->setText("255");
    }
    else if(text.length() == 3)
    {
        if(nextItem != NULL)
        {
            nextItem->setFocus();
            nextItem->selectAll();
        }
    }
}
