#ifndef WIFIDIALOG_H
#define WIFIDIALOG_H

#include <QDialog>

namespace Ui {
class WIFIDialog;
}

class WIFIDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit WIFIDialog(QWidget *parent = 0);
    ~WIFIDialog();
    
private:
    Ui::WIFIDialog *ui;
};

#endif // WIFIDIALOG_H
