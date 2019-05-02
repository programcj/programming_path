#include "mesnoticedialog.h"
#include "ui_mesnoticedialog.h"
#include "QDebug"
//Qt::FramelessWindowHint 来实现无边框窗口，用 Qt::Popup 来实现弹出式的窗口，用
//Qt::Tool 来实现工具窗口，
//Qt::CustomizeWindowHint 来关闭窗口标题栏以及与
//Qt::WindowCloseButton（添加关闭按钮），
//Qt::WindowMaximumButtonSize（添加最大化按钮）联用来建立只有关闭按钮和最大化按钮的窗口
//Qt::WindowStaysOnTopHint 使窗口永远在最前端等。
//Qt::WidgetAttribute 使窗口支持透明背景以及在关闭后主动销毁
MESNoticeDialog::MESNoticeDialog(QWidget *parent) :
    QDialog(parent,Qt::FramelessWindowHint|Qt::Popup),
    ui(new Ui::MESNoticeDialog)
{
    setAttribute(Qt::WA_TranslucentBackground); //透明处理
    setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);
    setGeometry(800-geometry().width(), 480-geometry().height(), geometry().width(), geometry().height());
    this->setMouseTracking ( true);//自动跟踪鼠标
}

MESNoticeDialog::~MESNoticeDialog()
{
    qDebug()<<"~MESNoticeDialog";
    delete ui;
}

void MESNoticeDialog::setText(const QString &str)
{
    ui->textEdit->setText(str);
}

void MESNoticeDialog::on_ptOK_clicked()
{
    close();
}

void MESNoticeDialog::mousePressEvent(QMouseEvent *e)
{
   int x=geometry().left();
   int y=geometry().top();
   int x1=geometry().right();
   int y1=geometry().bottom();

    //qDebug()<<"mousePressEvent:"<<e->pos().x()<<" "<<e->pos().y();

    if(e->pos().x()>x && e->pos().x()<x1 )
        if(e->pos().y()>y&& e->pos().y()<y1)
        {
            return;
        }
    close();
}

void MESNoticeDialog::mouseMoveEvent(QMouseEvent *e)
{
    //qDebug()<<"mouseMoveEvent:"<<e->pos().x()<<" "<<e->pos().y();
}

void MESNoticeDialog::focusOutEvent(QFocusEvent *e)
{
    //qDebug()<<"focusOutEvent:"<<e->gotFocus();
}
