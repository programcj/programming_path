#include <QDebug>
#include <QSignalMapper>
#include <QMouseEvent>
#include "../Public/public.h"
#include "inputpanelform.h"
#include "inputpannelcontext.h"

InputPanel::InputPanel(InputPannelContext *ipc,QWidget *parent)
                  :QWidget(parent, Qt::Tool|Qt::FramelessWindowHint),
                  //: QWidget(parent, Qt::Tool | Qt::WindowStaysOnTopHint),
                   lastFocusedWidget(0)
{
    inputform=new Ui::InputPanelForm;
    inputform->setupUi(this);
    QList<QPushButton *> btns=findChildren<QPushButton *>();

    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(saveFocusWidget(QWidget*,QWidget*)));
    QSignalMapper *myMapper=new QSignalMapper(this);
    for(int i=0;i<btns.size();i++)
    {
        if(btns[i]->objectName().startsWith("pushButton_key_"))
        {
            myMapper->setMapping(btns[i],btns[i]);
            connect(btns[i],SIGNAL(clicked()),myMapper,SLOT(map()));
        }
    }

    connect(myMapper,SIGNAL(mapped(QWidget*)),this,SLOT(btnClicked(QWidget*)));

    connect(this,SIGNAL(sendChar(QChar)),ipc,SLOT(charSlot(QChar)));
    connect(this,SIGNAL(sendInt(int)),ipc,SLOT(intSlot(int)));

    should_move=false;

    on_pushButton_abc_clicked();
}

void InputPanel::btnClicked(QWidget *w)
{
     QPushButton *btnp=static_cast<QPushButton *>(w);       
     emit sendChar(btnp->text().at(0));
}

//空格录入
void InputPanel::on_pushButton_space_clicked()
{
    emit sendChar(' ');
}
//回车 确定
void InputPanel::on_pushButton_OK_clicked()
{
    emit sendInt(55);
}

//回车 确定
void InputPanel::on_pushButton_number_ok_clicked()
{
   emit sendInt(55);
}

QWidget *InputPanel::getFocusedWidget()
{
    return lastFocusedWidget;
}

void InputPanel::saveFocusWidget(QWidget *oldFocus, QWidget *newFocus)
{
    if (newFocus != 0 && !this->isAncestorOf(newFocus)) {
        if(newFocus->objectName().startsWith("lineEdit_toolText")) {
        	lastFocusedWidget=0;
            return;
        }
        lastFocusedWidget = newFocus;       
    }
}

bool InputPanel::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::WindowActivate:
        if (lastFocusedWidget)
            lastFocusedWidget->activateWindow(); //  设置窗口所在的独立窗口为激活状态
        break;
    case QEvent::MouseButtonPress:
        mousePressEvent(e);
        break;
    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(e);
        break;
    case QEvent::MouseMove:
        mouseMoveEvent(e);
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

void InputPanel::mousePressEvent (QEvent *e)
{
    QMouseEvent *event=static_cast<QMouseEvent *>(e);
    if (event->button()!=Qt::LeftButton) return;
    this->should_move = true;
    this->widget_pos = this->pos();
    this->mouse_pos = event->globalPos();
}

void InputPanel::mouseReleaseEvent(QEvent *e)
{
    QMouseEvent *event=static_cast<QMouseEvent *>(e);
    if (event->button()!=Qt::LeftButton) return;
    this->should_move = false;
}

void InputPanel::mouseMoveEvent ( QEvent *e)
{
    QMouseEvent *event=static_cast<QMouseEvent *>(e);
    if (this->should_move)
    {
        QPoint pos = event->globalPos();
        int x = pos.x()-this->mouse_pos.x();
        int y = pos.y()-this->mouse_pos.y();
        QWidget::move(this->widget_pos.x()+x,this->widget_pos.y()+y);
    }
}

//大小写转换
void InputPanel::on_pushButton_capsLook_clicked()
{
    if(inputform->pushButton_key_a->text().startsWith("a"))
    {
        //当前为小写
        QList<QPushButton *> btns=findChildren<QPushButton *>();
        for(int i=0;i<btns.size();i++)
        {
           if(btns[i]->objectName().startsWith("pushButton_key_"))
           {
               btns[i]->setText(btns[i]->text().toUpper());
           }
        }
        inputform->pushButton_capsLook->setText("小写");
    } else
    {
         QList<QPushButton *> btns=findChildren<QPushButton *>();
         for(int i=0;i<btns.size();i++)
         {
            if(btns[i]->objectName().startsWith("pushButton_key_"))
            {
                btns[i]->setText(btns[i]->text().toLower());
            }
         }
         inputform->pushButton_capsLook->setText("大写");
    }
}

//隐藏
void InputPanel::on_pushButton_hide_clicked()
{
    hide();
}
//隐藏
void InputPanel::on_pushButton_number_hide_clicked()
{
    hide();
}

//变形为小写键盘
void InputPanel::on_pushButton_123_clicked()
{
    QRect size=geometry();
    size.setWidth(inputform->widget_2->geometry().width());
    size.setHeight(inputform->widget_2->geometry().height());
    setGeometry(size);
    inputform->widget->hide();
    inputform->widget_2->show();
    inputform->widget_2->setGeometry(QRect(0,0,inputform->widget_2->geometry().width(),inputform->widget_2->geometry().height()));
}

//变形为大写键盘
void InputPanel::on_pushButton_abc_clicked()
{
    QRect size=geometry();
    size.setWidth(inputform->widget->geometry().width());
    size.setHeight(inputform->widget->geometry().height());
    setGeometry(size);
    inputform->widget_2->hide();
    inputform->widget->show();
}

void InputPanel::on_pushButton_del_clicked()
{
    emit sendInt(54);
}

void InputPanel::on_pushButton_number_del_clicked()
{
    emit sendInt(54);
}
