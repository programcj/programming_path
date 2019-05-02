#include "mainhomepage.h"
#include "ui_mainhomepage.h"

MainHomePage::MainHomePage(QWidget *parent) :
		QWidget(parent), ui(new Ui::MainHomePage)
{
	ui->setupUi(this);

	connect(&OrderMainOperation::GetInstance(),
            SIGNAL(OnSignalUpdateMainOrder()),
            this, SLOT(OnUpdateMainOrder()));

    connect(UIPageButtonStatus::GetInstance(),
            SIGNAL(onSigFunButtonChange(ButtonStatus*)),
            this, SLOT(onFunButtonChange(ButtonStatus*)));

	pcsnum = 0; //工单件号
	allproduct = 0;
	dispatchNo = 0;
    ShowInfo();
}

MainHomePage::~MainHomePage()
{
	delete ui;
}

void MainHomePage::OnUpdateMainOrder()
{
    ShowInfo();
}

void MainHomePage::onFunButtonChange(ButtonStatus *bt)
{
    ShowInfo();
}

void MainHomePage::paintEvent(QPaintEvent *event)
{
	float badrate;      //次品率
	float productnrate; //生产率

	QPainter painter(this);
	QColor m_Color[10];

	m_Color[0] = Qt::darkGreen;
	m_Color[1] = Qt::red;
	m_Color[2] = Qt::darkBlue;
	m_Color[3] = Qt::darkGray;
	m_Color[4] = Qt::darkYellow;

	int m_Result[10]={0};


	m_Result[0] = classgood; //良品
	m_Result[1] = classbad; //次品
	m_Result[2] = classinspect; //巡机
	//m_Result[3] = classpoinsh; //打磨

	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen( QPen(Qt::black, 1) );
	int FULL_CIRCLE = 16*360;
    int RADIUS = 100;
    QRect rect(10, 10, 200, 200);

    //画一个背景圆形
    painter.setBrush( Qt::gray );
    painter.drawEllipse(rect);

	int pos = 0;
	int angle;
	for(int i=0; i<10; i++)
	{
		if( 0 == m_Result[i] )
			continue;
		painter.setBrush( m_Color[i] );
		double persent = (double)m_Result[i] / classAll;
		angle = FULL_CIRCLE * persent;

		//画出各个对应的扇形
		double abc = 3.14 * 2 * (double)(pos + angle/2) / FULL_CIRCLE;
		double tx = 80 * qCos(abc)+10 + RADIUS;
		double ty = -80 * qSin(abc)+10 + RADIUS;
		painter.drawPie(rect, pos, angle);
		//在扇形上写注释（投票数和百分比）
		QString temp;
		//temp.sprintf(" (%d) ", m_Result[i]);
		if(i ==0)
		temp = "良品";
		else if (i ==1)
		temp = "次品";
		else if (i==2)
		{
			temp = "巡机";
		}
		painter.drawText(tx-20, ty-10, temp);
		temp.sprintf("%0.1lf%%", persent*100);
		painter.drawText(tx-20, ty, temp);
		pos += angle;
    }
}

//显示信息
void MainHomePage::ShowInfo()
{
    //派工单号 -
    //产品编号 -
    //产品名称 -

    //产品-派工数 模次数 良品数 生产数 次品数

    //本班-生产数 良品数 次品数 次品率

    Order order;
    if (Order::query(order, 1))
    {
    	order.getBoy();
    }
//
    QMap<QString, QString> orderMap;
    QMap<QString, QString> orderBoyMap;

    OrderMainOperation::OrderToQMap(order, orderMap, pcsnum,
            orderBoyMap);

    orderMap.unite(orderBoyMap);

    ui->label_order->setText("派工单号:"+orderMap["派工单号"]);
    ui->label_num->setText("产品编号:"+orderMap["产品编号"]);
    ui->label_name->setText("产品描述:"+orderMap["产品描述"]);

    ui->tableWidgetCount->setItem(0,0,new QTableWidgetItem(orderMap["派工数量"]));
    ui->tableWidgetCount->setItem(0,1,new QTableWidgetItem(orderMap["模次"]));
    ui->tableWidgetCount->setItem(0,2,new QTableWidgetItem(orderMap["良品数"]));
    ui->tableWidgetCount->setItem(0,3,new QTableWidgetItem(orderMap["生产总数"]));
    ui->tableWidgetCount->setItem(0,4,new QTableWidgetItem(orderMap["次品总数"]));

    ui->tableWidgetCount->item(0,0)->setTextAlignment(Qt::AlignHCenter);
    ui->tableWidgetCount->item(0,1)->setTextAlignment(Qt::AlignHCenter);
    ui->tableWidgetCount->item(0,2)->setTextAlignment(Qt::AlignHCenter);
    ui->tableWidgetCount->item(0,3)->setTextAlignment(Qt::AlignHCenter);
    ui->tableWidgetCount->item(0,4)->setTextAlignment(Qt::AlignHCenter);

    classAll     = orderMap["本班生产总数"].toInt();
    classbad     = orderMap["本班次品总数"].toInt();
    classinspect = orderMap["本班巡机数"].toInt();
    classpoinsh  = orderMap["本班打磨数"].toInt();
   	classgood    = orderMap["本班良品数"].toInt();


    ui->tableWidgetClass->setItem(0,0,new QTableWidgetItem(orderMap["本班生产总数"]));
    ui->tableWidgetClass->setItem(0,1,new QTableWidgetItem(orderMap["本班良品数"]));
    ui->tableWidgetClass->setItem(0,2,new QTableWidgetItem(orderMap["本班次品总数"]));
    ui->tableWidgetClass->setItem(0,3,new QTableWidgetItem(orderMap["本班次品率"]));

    ui->tableWidgetClass->item(0,0)->setTextAlignment(Qt::AlignHCenter);
    ui->tableWidgetClass->item(0,1)->setTextAlignment(Qt::AlignHCenter);
    ui->tableWidgetClass->item(0,2)->setTextAlignment(Qt::AlignHCenter);
    ui->tableWidgetClass->item(0,3)->setTextAlignment(Qt::AlignHCenter);

    if (AppInfo::GetInstance().getHaveStopCard()) //有停机卡就不记模
    {
           ui->runStatus->setText("停机");
           ui->runStatus->setStyleSheet("border-radius: 100px;\
                                        background-color: rgb(0, 172, 0);\
                                        background-color: red;\
                                        font:bold 50px;\
                                        color: rgb(255, 255, 255);");
    }
    else
    {
           ui->runStatus->setText("运行");
           ui->runStatus->setStyleSheet("border-radius: 100px;\
                                        background-color: rgb(0, 172, 0);\
                                        background-color:green;\
                                        font:bold 50px;\
                                        color: rgb(255, 255, 255);");
     }

    update(QRect(10, 10, 200, 200));
}

//上一件
void MainHomePage::on_btItemUp_clicked()
{
	if(pcsnum>0)
        pcsnum--;
    ShowInfo();

}
//下一件
void MainHomePage::on_btItemNext_clicked()
{
    if(pcsnum<OrderMainOperation::GetInstance().mainOrderCache.orderBoyList.size()-1)
        pcsnum++;
    ShowInfo();
}
