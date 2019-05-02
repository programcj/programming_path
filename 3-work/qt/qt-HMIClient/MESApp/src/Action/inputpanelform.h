#ifndef INPUTPANELFORM_H
#define INPUTPANELFORM_H

#include <QWidget>
#include <QList>
#include <QChar>
#include <QString>
#include <QPoint>
#include "ui_inputpanelform.h"

class InputPannelContext;

namespace Ui {
    class InputPanelForm;
}

class InputPanel : public QWidget
{
    Q_OBJECT
public:
    explicit InputPanel(InputPannelContext *ipc,QWidget *parent=0 );


    QWidget *getFocusedWidget();
signals:
    void sendChar(QChar ch);
    void sendInt(int key);

protected:
    bool event(QEvent *e);

private slots:
    void saveFocusWidget(QWidget *oldFocus, QWidget *newFocus);
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

private:
    Ui::InputPanelForm *inputform;

    int caps;
    int num;

    QWidget *lastFocusedWidget;

    bool should_move;
    QPoint  mouse_pos;
    QPoint  widget_pos;

    void mousePressEvent(QEvent *e);
    void mouseReleaseEvent(QEvent *e);
    void mouseMoveEvent(QEvent *e);

};

#endif // INPUTPANELFORM_H
