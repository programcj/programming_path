#ifndef TOOLTEXTINPUTDIALOG_H
#define TOOLTEXTINPUTDIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include "../Public/public.h"

namespace Ui {
class ToolTextInputDialog;
}

class ToolTextInputDialog : public QDialog
{
    Q_OBJECT
    
public:
    enum Key {
        KeyEnter,
        KeyHide
    };

    enum InputType {
        none,//"--输入普通字符
        typeText,//"--输入普通字符
        typeNumber,//"--数字格式,正整数 0123456789
        typeNumberSigned,//"--有符号数字格式,整数 +-0123456789
        typeNumberDecimal//"--可以带小数点的浮点格式 .0123456789
    };

    explicit ToolTextInputDialog(QWidget *parent = 0, InputType inputType=none, QString text="",const QString &title="");
    ~ToolTextInputDialog();

    const QString text();
    //设定输入类型
    void setInputMask(const QString &inputMask);

    void sendInt(int v);
    void sendChar(QChar ch);
private slots:
    void btnClicked(QWidget *w);
    void on_pushButton_hide_clicked();

    void on_pushButton_capsLook_clicked();

    void on_pushButton_number_hide_clicked();

    void on_pushButton_space_clicked();

    void on_pushButton_OK_clicked();

    void on_pushButton_number_ok_clicked();

    void on_pushButton_123_clicked();

    void on_pushButton_abc_clicked();

    void on_pushButton_del_clicked();

    void on_pushButton_number_del_clicked();

    void on_pushButton_clear_clicked();

private:
    QSignalMapper *myMapper;
    QString filter;
    InputType m_inputType;
    Ui::ToolTextInputDialog *ui;
};

#endif // TOOLTEXTINPUTDIALOG_H
