#include "mainformbadpcs.h"
#include "ui_mainformbadpcs.h"
#include "dialogtoast.h"
#include "dialogswingcard.h"
#include "../dao/mesdispatchorder.h"
#include "../netinterface/mesmsghandler.h"
#include "../tools/mestools.h"

static char *text[]={
    "缺料",
    "色差",
    "黑点",
    "变形",
    "烧焦",
    "划痕",
    "瑕疵",
    "缩水",
    "料花",
    "尺寸",
    "夹纹",
    "缺省",

    "缺料",
    "色差",
    "黑点",
    "变形",
    "烧焦",
    "划痕",
    "瑕疵",
    "缩水",
    "料花",
    "尺寸",
    "夹纹",
    "缺省",
     "缺省"
};

MainFormBadpcs::MainFormBadpcs(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainFormBadpcs)
{
    ui->setupUi(this);
    page=0;

    for(int i=0;i<20;i++) {
        uiGetBadName(i)->setText(QString("%1").arg(text[i]));

        uiGetBadNewValue(i)->setText("");
        ui->comboBox_BadPCSName->addItem(QString("%1 %2").arg(i+1).arg(uiGetBadName(i)->text()));
    }

    on_btPrevPage_clicked();

    connect(MESDispatchOrder::getInstance(),SIGNAL(sigMainOrderUpdate()),this,SLOT(onMainDispatchOrderUpdate()));

    QPushButton *buttons[] = {
        ui->btInput_0,ui->btInput_1,ui->btInput_2,ui->btInput_3,ui->btInput_4,ui->btInput_5,ui->btInput_6,ui->btInput_7,ui->btInput_8,
        ui->btInput_9,ui->btInput_add,ui->btInput_sub
    };
    signalMapper = new QSignalMapper(this);
    for(int i=0;i<12;i++){
        signalMapper->setMapping(buttons[i],buttons[i]);
        connect(buttons[i],SIGNAL(clicked()), signalMapper, SLOT(map()));
    }
    connect(signalMapper,SIGNAL(mapped(QWidget*)),this,SLOT(onInputClicked(QWidget*)));
}

MainFormBadpcs::~MainFormBadpcs()
{
    delete ui;
}

void MainFormBadpcs::onMainDispatchOrderUpdate()
{
    QString MO_DispatchNo=MESDispatchOrder::onGetMainDispatchNo();
    QString PCS_ItemNO=ui->lineEdit_PCS_ItemNO->text();

    //if main order change
    if(PCS_ItemNO.length()>0){
        QString sql=QString("select * from %1 where PCS_ItemNO='%2' and MO_DispatchNo='%3'")
                .arg(TABLE_DORDER_ITEM)
                .arg(PCS_ItemNO)
                .arg(MO_DispatchNo);
        QJsonObject json;
        sqlite::SQLiteBaseHelper::getInstance().selectOne(sql,json);
        if(json.contains("PCS_ItemNO")) {
            ui->lineEdit_PCS_ItemNO->setText(json["PCS_ItemNO"].toString());
            ui->lineEdit_PCS_ItemName->setText(json["PCS_ItemName"].toString());
            ui->lineEdit_PCS_DispatchQty->setText(json["PCS_DispatchQty"].toString());
            ui->lineEdit_PCS_BadQty->setText(json["PCS_BadQty"].toString());

            int TotalNum=MESDispatchOrder::onGetDispatchNoMoTotalNum(MO_DispatchNo);
            int PCS_SocketNum1=json["PCS_SocketNum1"].toString().toInt();
            if(PCS_SocketNum1==0){
                PCS_SocketNum1=1;
            }
            ui->lineEdit_Count->setText(QString("%1").arg(TotalNum*PCS_SocketNum1));
            return;
        }
    }
    on_btPrevPage_clicked();
}

void MainFormBadpcs::onShowBadInfo(QString PCS_BadDataString)
{
    //QString PCS_BadDataString=json["PCS_BadData"].toString();
    QByteArray sss=PCS_BadDataString.toLatin1();

    QJsonParseError json_error;
    QJsonDocument parse_doucment = QJsonDocument::fromJson(sss, &json_error);
    QJsonArray array;
    if(json_error.error == QJsonParseError::NoError) {
        array=parse_doucment.array();
    }
    for(int i=0;i<20;i++){
        QString value="";
        if(i<array.size()){
            value=QString("%1").arg(array[i].toInt());
        }
        uiGetBadValue(i)->setText(value);
    }
}

/**
 * 上一页
 * @brief MainFormBadpcs::on_btPrevPage_clicked
 */
void MainFormBadpcs::on_btPrevPage_clicked()
{
    QString MO_DispatchNo=MESDispatchOrder::onGetMainDispatchNo();
    int length=MESDispatchOrder::onOrderItemSize(MO_DispatchNo);
    QJsonArray jsonArray;
    MESDispatchOrder::onGetDispatchNoItems(MO_DispatchNo,jsonArray);
    if(page>0){
        page--;
    }
    QJsonObject json;
    if(jsonArray.size()>page){
        json=jsonArray.at(page).toObject();
    }
    ui->lineEdit_PCS_ItemNO->setText(json["PCS_ItemNO"].toString());
    ui->lineEdit_PCS_ItemName->setText(json["PCS_ItemName"].toString());
    ui->lineEdit_PCS_DispatchQty->setText(json["PCS_DispatchQty"].toString());
    ui->lineEdit_PCS_BadQty->setText(json["PCS_BadQty"].toString());

    int count=MESDispatchOrder::onGetDispatchNoItemCount(MO_DispatchNo,json["PCS_ItemNO"].toString());

    ui->lineEdit_Count->setText(QString("%1").arg(count));

    if(length>0){
        QString info=QString("%1/%2").arg(page+1).arg(length);
        ui->label_PageInfo->setText(info);
    } else {
        ui->label_PageInfo->setText("0/0");
    }
    onShowBadInfo(json["PCS_BadData"].toString());
}

/**
 * 下一页
 * @brief MainFormBadpcs::on_btNextPage_clicked
 */
void MainFormBadpcs::on_btNextPage_clicked()
{
    QString MO_DispatchNo=MESDispatchOrder::onGetMainDispatchNo();
    int length=MESDispatchOrder::onOrderItemSize(MO_DispatchNo);
    QJsonArray jsonArray;
    MESDispatchOrder::onGetDispatchNoItems(MO_DispatchNo,jsonArray);
    if(page+1<jsonArray.size()){
        page++;
    }
    QJsonObject json;
    if(jsonArray.size()>page){
        json=jsonArray.at(page).toObject();
    }
    ui->lineEdit_PCS_ItemNO->setText(json["PCS_ItemNO"].toString());
    ui->lineEdit_PCS_ItemName->setText(json["PCS_ItemName"].toString());
    ui->lineEdit_PCS_DispatchQty->setText(json["PCS_DispatchQty"].toString());
    ui->lineEdit_PCS_BadQty->setText(json["PCS_BadQty"].toString());

    int count=MESDispatchOrder::onGetDispatchNoItemCount(MO_DispatchNo,json["PCS_ItemNO"].toString());

    ui->lineEdit_Count->setText(QString("%1").arg(count));

    if(length>0){
        QString info=QString("%1/%2").arg(page+1).arg(length);
        ui->label_PageInfo->setText(info);
    } else {
        ui->label_PageInfo->setText("0/0");
    }
    onShowBadInfo(json["PCS_BadData"].toString());
}

/**
 * save 次品录入
 * @brief MainFormBadpcs::on_btSaveBadNumber_clicked
 */
void MainFormBadpcs::on_btSaveBadNumber_clicked()
{
    {
        if(!MESMsgHandler::getInstance()->isConnected()){
            DialogToast dialog("未连接到服务器");
            dialog.show(1000);
            return ;
        }
    }
    QString DispatchNo=MESDispatchOrder::onGetMainDispatchNo();
    QString PCS_ItemNO=ui->lineEdit_PCS_ItemNO->text();
    {
        int newSum=0;
        for(int i=0;i<20;i++) {
            newSum+=uiGetBadNewValue(i)->text().toInt();
        }
        if(newSum==0) {
            DialogToast dialog("未输入次品");
            dialog.show(2000);
            return ;
        }
    }
    {
        QString DispatchNo=MESDispatchOrder::onGetMainDispatchNo();
        int count= MESDispatchOrder::onGetDispatchNoItemCount(DispatchNo, ui->lineEdit_PCS_ItemNO->text());
        int inputCount=0;
        for(int i=0;i<20;i++) {
            inputCount+=uiGetBadValue(i)->text().toInt();
            inputCount+=uiGetBadNewValue(i)->text().toInt();
        }
        if(inputCount > count) {
            DialogToast dialog("次品超过生产总数");
            dialog.show(2000);
            return ;
        }
    }
    QString CardID="";
    {
        //刷卡
        DialogSwingCard dialog;
        QString btName="次品提交";

        dialog.setTitleText(btName);
        if (dialog.exec() == QDialog::Rejected)
            return;
        //依据功能按钮寻找配置信息
        if(dialog.CardID.length()==0){
            DialogToast dialog("没有刷卡!");
            dialog.show(2000);
            return;
        }
        CardID=dialog.CardID;
    }
    QString PCS_BadDataString="";
    int curCount=0;
    {
        QJsonObject json;
        MESDispatchOrder::onGetDispatchNoItem(DispatchNo,PCS_ItemNO, json);
        QString PCS_BadDataStr=json["PCS_BadData"].toString();  ///每件产品的次品数据（对应次品原因的数据）

        QJsonParseError json_error;
        QJsonDocument parse_doucment = QJsonDocument::fromJson(PCS_BadDataStr.toLatin1(), &json_error);
        if(json_error.error == QJsonParseError::NoError) {
            QJsonArray array=parse_doucment.array();
            int badCount=0;

            for(int i=0,j=array.size();i<j;i++){
                int newValue=uiGetBadNewValue(i)->text().toInt();
                int resValue=array[i].toInt()+newValue;
                curCount+=newValue;

                array.replace(i,resValue);
                badCount+=resValue;
            }
            //save to database
            MESDispatchOrder::onDispatchItemSaveBadData(DispatchNo,PCS_ItemNO, array);
            MESDispatchOrder::onDispatchItemAddBadQty(DispatchNo,PCS_ItemNO, curCount);

            PCS_BadDataString=QString(QJsonDocument(array).toJson());
        }
    }
    {
        QJsonArray badArray;
        for(int i=0;i<20;i++) {
            badArray.append(uiGetBadNewValue(i)->text().toInt());
            uiGetBadNewValue(i)->setText("");
        }

        QString msg;
        QJsonObject json;
        QJsonObject item;
        QJsonArray FDAT_Data;

        json.insert("FDAT_DispatchNo",DispatchNo);
        json.insert("FDAT_CardID", CardID);
        json.insert("FDAT_CardDate",MESTools::GetCurrentDateTimeStr());
        json.insert("FDAT_MultiNum",1);

        item.insert("Bad_ItemNo",PCS_ItemNO);
        item.insert("Bad_BadQty",curCount);
        item.insert("Bad_BadData",badArray);

        FDAT_Data.append(item);

        json.insert("FDAT_Data",FDAT_Data);

        if(MESMsgHandler::getInstance()->submitBadData(json,msg) ) {
            DialogToast dialog("保存成功!");
            dialog.show(2000);
        } else {
            DialogToast dialog(msg);
            dialog.show(2000);
            return;
        }
    }
    //////////
    onShowBadInfo(PCS_BadDataString);
}

void MainFormBadpcs::on_pushButton_Ok_clicked()
{
    int index=ui->comboBox_BadPCSName->currentIndex();
    int newValue=ui->lineEdit_BadNumber->text().toInt();
    int value=uiGetBadValue(index)->text().toInt();

    ui->lineEdit_BadNumber->setText("");

    if(value+newValue<0){
        DialogToast dialog("值小于0");
        dialog.show(2000);
        return ;
    }
    QString DispatchNo=MESDispatchOrder::onGetMainDispatchNo();
    int count= MESDispatchOrder::onGetDispatchNoItemCount(DispatchNo, ui->lineEdit_PCS_ItemNO->text());
    int inputCount=0;
    for(int i=0;i<20;i++){
        inputCount+=uiGetBadValue(i)->text().toInt();
        if(index==i) {
            inputCount+=newValue;
        } else {
            inputCount+=uiGetBadNewValue(i)->text().toInt();
        }
    }
    if(inputCount>count){
        DialogToast dialog("次品超过生产总数");
        dialog.show(2000);
        return ;
    }
    if(newValue>0)
        uiGetBadNewValue(index)->setText(QString("+%1").arg(newValue));
    if(newValue<0)
        uiGetBadNewValue(index)->setText(QString("%1").arg(newValue));
    if(newValue==0)
        uiGetBadNewValue(index)->setText("");
}

void MainFormBadpcs::on_pushButton_No_clicked()
{
    int index=ui->comboBox_BadPCSName->currentIndex();
    ui->lineEdit_BadNumber->setText("");
    uiGetBadNewValue(index)->setText("");
}

void MainFormBadpcs::onInputClicked(QWidget *widget)
{
    QPushButton *button= (QPushButton*)widget;
    QString textName=button->objectName();
    textName=textName.right(1);
    if(textName=="d"){ //add
        textName="+";
        int v= ui->lineEdit_BadNumber->text().toInt();
        if(v<0) {
            v=0-v;
        }
        textName=QString("+%1").arg(v);
    } else if(textName=="b"){ //sub
        textName="-";
        int v= ui->lineEdit_BadNumber->text().toInt();
        if(v>0) {
            v=0-v;
        }
        textName=QString("%1").arg(v);
    } else {
        textName=ui->lineEdit_BadNumber->text()+textName;
    }
    ui->lineEdit_BadNumber->setText(textName);
}

QLabel *MainFormBadpcs::uiGetBadName(int index)
{
    QLabel *labels[] = {
        ui->label_B_1,ui->label_B_2,ui->label_B_3,ui->label_B_4,ui->label_B_5,ui->label_B_6,ui->label_B_7,
        ui->label_B_8,ui->label_B_9,ui->label_B_10,ui->label_B_11,ui->label_B_12,ui->label_B_13,
        ui->label_B_14 ,ui->label_B_15,ui->label_B_16,ui->label_B_17,ui->label_B_18,
        ui->label_B_19,ui->label_B_20
    };
    return labels[index];
}

QLineEdit *MainFormBadpcs::uiGetBadValue(int index)
{
    QLineEdit *lineEdits[]= {
        ui->lineEdit_B_1, ui->lineEdit_B_2, ui->lineEdit_B_3, ui->lineEdit_B_4, ui->lineEdit_B_5,
        ui->lineEdit_B_6, ui->lineEdit_B_7, ui->lineEdit_B_8, ui->lineEdit_B_9, ui->lineEdit_B_10,
        ui->lineEdit_B_11,ui->lineEdit_B_12,ui->lineEdit_B_13,ui->lineEdit_B_14,ui->lineEdit_B_15,
        ui->lineEdit_B_16,ui->lineEdit_B_17,ui->lineEdit_B_18,ui->lineEdit_B_19,ui->lineEdit_B_20
    };
    return lineEdits[index];
}

QLabel *MainFormBadpcs::uiGetBadNewValue(int index)
{
    QLabel *labels[] = {
        ui->label_New_1, ui->label_New_2,ui->label_New_3,ui->label_New_4,ui->label_New_5,ui->label_New_6,ui->label_New_7,ui->label_New_8,
        ui->label_New_9,ui->label_New_10,ui->label_New_11,ui->label_New_12,ui->label_New_13,ui->label_New_14,ui->label_New_15,ui->label_New_16,
        ui->label_New_17,ui->label_New_18,ui->label_New_19,ui->label_New_20
    };
    return labels[index];
}
