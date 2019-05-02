#include "mainformfirst.h"
#include "ui_mainformfirst.h"
#include "../netinterface/mesmqttclient.h"
#include "../dao/sqlitebasehelper.h"
#include "../dao/mesdispatchorder.h"

MainFormFirst::MainFormFirst(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainFormFirst)
{
    ui->setupUi(this);

    orderIndex=0;
    orderItemIndex=0;

    connect(MESDispatchOrder::getInstance(),SIGNAL(sigMainOrderUpdate()),this,SLOT(onMainDispatchOrderUpdate()));

    onShowOrder(orderIndex); 
}

MainFormFirst::~MainFormFirst()
{
    delete ui;
}

void MainFormFirst::onMainDispatchOrderUpdate()
{
    if(orderIndex!=0)
        return;

    onShowOrder(orderIndex);
}

/**
 * 上一单
 * 需要记录首单，然后，每一单显示下去，如果达到了未尾须要处理，如果到0则显示首单
 * @brief MainFormFirst::on_btItemUp_clicked
 */
void MainFormFirst::on_btItemUp_clicked()
{
    //MESMqttClient::GetInstance()->sendMsg(QString("/aaaa"),arr);
    //long orderSize = MESDispatchOrder::onOrderSize();
    if(this->orderIndex>0) {
        orderIndex--;
        return ;
    }
    onShowOrder(orderIndex);
}

/**
 * 下一单
 * @brief MainFormFirst::on_btItemNext_clicked
 */
void MainFormFirst::on_btItemNext_clicked()
{
    long orderSize = MESDispatchOrder::onOrderSize();
    if(this->orderIndex+1>=orderSize)
        return ;
    orderSize++;
    onShowOrder(orderIndex);
}

void MainFormFirst::on_btItemUpItem_clicked()
{
    QString MO_DispatchNo=ui->edit_MO_DispatchNo->text();
    //int size=MESDispatchOrder::onOrderItemSize(MO_DispatchNo);
    if(orderItemIndex==0)
        return;
    onShowOrderItem(MO_DispatchNo,orderItemIndex-1);
}

void MainFormFirst::on_btItemNextItem_clicked()
{
     QString MO_DispatchNo=ui->edit_MO_DispatchNo->text();
    int size=MESDispatchOrder::onOrderItemSize(MO_DispatchNo);
    if(orderItemIndex+1>=size)
        return;

    onShowOrderItem(ui->edit_MO_DispatchNo->text(),orderItemIndex+1);
}

void MainFormFirst::onShowOrder(int index)
{
    QJsonObject jsonOrder;
    //QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    QString sql=QString("select * from %1 order by %2 limit %3,1")
            .arg(TABLE_DISPATCH_ORDER)
            .arg(TABLE_FIELD_CREATED_TIME)
            .arg(index);

    if(sqlite::SQLiteBaseHelper::getInstance().selectOne(sql,jsonOrder)){
        qDebug()<< QString(QJsonDocument(jsonOrder).toJson());
       QJsonValue value= jsonOrder["MO_DispatchNo"];

        ui->edit_MO_DispatchNo->setText(jsonOrder["MO_DispatchNo"].toString());
        ui->edit_MO_ProcCode->setText(jsonOrder["MO_ProcCode"].toString());
        ui->edit_MO_ProcName->setText(jsonOrder["MO_ProcName"].toString());
        ui->label_MO_TotalNum->setText(jsonOrder["MO_TotalNum"].toString());
        if(index==0) {
            ui->label_MainOrderFlag->setText("主单");
        } else {
            ui->label_MainOrderFlag->setText("次单");
        }
    } else {
        ui->edit_MO_DispatchNo->setText("");
        ui->edit_MO_ProcCode->setText("");
        ui->edit_MO_ProcName->setText("");
        ui->label_MO_TotalNum->setText("");
        ui->label_MainOrderFlag->setText("");
    }

    onShowOrderItem(jsonOrder["MO_DispatchNo"].toString(), 0);
}

void MainFormFirst::onShowOrderItem(QString MO_DispatchNo, int index)
{
    QJsonObject json;  
    QString sql=QString("select * from %1 where MO_DispatchNo='%2' order by %3 limit %4,1")
            .arg(TABLE_DORDER_ITEM)
            .arg(MO_DispatchNo)
            .arg(TABLE_FIELD_CREATED_TIME)
            .arg(index);

    orderItemIndex=index;

    if(sqlite::SQLiteBaseHelper::getInstance().selectOne(sql,json)){
        ui->edit_PCS_MO->setText(json["PCS_MO"].toString());
        ui->edit_PCS_ItemNo->setText(json["PCS_ItemNO"].toString());
        ui->edit_PCS_MouldNo->setText(json["PCS_MouldNo"].toString());
        ui->edit_PCS_DispatchQty->setText(json["PCS_DispatchQty"].toString());
        ui->edit_PCS_SocketNum1->setText(json["PCS_SocketNum1"].toString());
        ui->edit_PCS_BadData->setText(json["PCS_BadQty"].toString());
        int TotalNum=MESDispatchOrder::onGetDispatchNoMoTotalNum(MO_DispatchNo);
        int PCS_SocketNum1=json["PCS_SocketNum1"].toInt();
        if(PCS_SocketNum1==0){
            PCS_SocketNum1=1;
        }
        ui->edit_Count->setText(QString("%1").arg(TotalNum*PCS_SocketNum1));
    } else {
        ui->edit_PCS_MO->setText("");
        ui->edit_PCS_ItemNo->setText("");
        ui->edit_PCS_MouldNo->setText("");
        ui->edit_PCS_DispatchQty->setText("");
        ui->edit_PCS_SocketNum1->setText("");
        ui->edit_PCS_BadData->setText("");
        ui->edit_Count->setText("");
    }
}
