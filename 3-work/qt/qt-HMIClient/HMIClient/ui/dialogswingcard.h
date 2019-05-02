#ifndef DIALOGSWINGCARD_H
#define DIALOGSWINGCARD_H

#include <QDialog>
#include <QDebug>
#include "../hal/halrfidread.h"

namespace Ui {
class DialogSwingCard;
}

class DialogSwingCard : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSwingCard(QWidget *parent = 0);
    ~DialogSwingCard();

    void setTitleText(const QString &text);

    QString CardID;

private slots:
    void on_pushButton_OK_clicked();
    void slotsReadrfiduid(QString uid);

private:
    HALRFIDRead *rfidRead;
    Ui::DialogSwingCard *ui;
};

#endif // DIALOGSWINGCARD_H
