#include "mainformordermanagement.h"
#include "ui_mainformordermanagement.h"
#include "dialogswingcard.h"
#include "dialogtoast.h"

#include "../appconfig.h"
#include "../application.h"
#include "../dao/mesdispatchorder.h"
#include "../dao/brushcard.h"
#include "../dao/sqlitebasehelper.h"
#include "../netinterface/mesmsghandler.h"
#include "../tools/mestools.h"

struct ButtonStatus {
    QPushButton *button;
    QString text;
    int CardType;
    bool IsStopCard;
};

#define CONFIG_IS_STOP_CARD     "IsStopCard"
#define CONFIG_IS_BEGIN_END     "IsBeginEnd"

///上班、下班、交班、换模、换料、换单、待人、待料、换班、
/// 机器故障、模具故障、辅设故障、无订单、同步工单、其它
///没有工单时就不能操作，只能上下班
////////////////////////////////////////////////////////////////////////////////////////
//CC_DispatchNo 派工单号
//CC_CardID 卡号
//CC_CardType   刷卡原因编号[功能ID]
//CC_CardDate 刷卡数据产生时间
//CC_IsBeginEnd 刷卡开始或结束标记,0表示开始,1表示结束
////////////////////////////////////////////////////////////////////////////////////////
MainFormOrderManagement::MainFormOrderManagement(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainFormOrderManagement)
{
    ui->setupUi(this);

    orderItemIndex=0;

    QPushButton *buttons[] = {
        ui->pushButton_1,ui->pushButton_2,ui->pushButton_3,ui->pushButton_4, ui->pushButton_5,ui->pushButton_6,  ui->pushButton_7,
        ui->pushButton_8,ui->pushButton_9,  ui->pushButton_10,  ui->pushButton_11,ui->pushButton_12, ui->pushButton_13
    } ;

    signalMapper=new QSignalMapper(this);
    for(int i=0;i<13; i++){
        qDebug() << buttons[i]->objectName();
    }
    for(int i=0;i<13; i++){
        signalMapper->setMapping(buttons[i], buttons[i]);
        connect(buttons[i],SIGNAL(clicked()), signalMapper, SLOT(map()));
    }

    connect(signalMapper,SIGNAL(mapped(QWidget*)),this,SLOT(doClicked(QWidget*)));
    onShowAllFunButton();
    connect(MESDispatchOrder::getInstance(),SIGNAL(sigMainOrderUpdate()),this,SLOT(onMainDispatchOrderUpdate()));

    onShowMainDispatchOrder();
}

MainFormOrderManagement::~MainFormOrderManagement()
{
    delete signalMapper;
    delete ui;
}

QJsonObject MainFormOrderManagement::configInfoLoad(bool defaultConfig) {
    struct ButtonStatus buttons[] = {
    { ui->pushButton_1,"上班",1, false},
    { ui->pushButton_2,"下班",2, false },
    { ui->pushButton_3, "交班",3, true },
    { ui->pushButton_4, "换模",4, true },
    { ui->pushButton_5, "换料",5, true },
    { ui->pushButton_6, "换单",6,  true },
    { ui->pushButton_7, "待人",7, true },
    { ui->pushButton_8, "待料",8, true },
    { ui->pushButton_9, "换班",9, true },
    { ui->pushButton_10, "机器故障",10, true },
    { ui->pushButton_11, "模具故障", 11, true },
    { ui->pushButton_12, "辅设故障", 12, true },
    { ui->pushButton_13, "无订单", 13, true }
} ;

    QFile file(FILE_NAME_FUNCARD);
    QJsonObject jsonConfig;

    if(defaultConfig || !file.exists()){
        for(int i=0;i<13; i++){
            QJsonObject itemJson;
            itemJson.insert(CONFIG_IS_STOP_CARD, buttons[i].IsStopCard);
            itemJson.insert("CardType",buttons[i].CardType);
            itemJson.insert(CONFIG_IS_BEGIN_END,1); /// 0-start;  1-end
            itemJson.insert("Name", buttons[i].button->text());
            itemJson.insert("StartTime",MESTools::GetCurrentDateTimeStr());
            itemJson.insert("StopTime",MESTools::GetCurrentDateTimeStr());
            jsonConfig.insert(buttons[i].button->objectName(), itemJson);
        }
    }

    if( file.exists() && defaultConfig==false) {
        if(file.open(QIODevice::ReadWrite | QIODevice::Text )) {
            QTextStream in(&file);
            QString text=in.readAll();
            file.close();

            QJsonParseError json_error;
            QJsonDocument parse_doucment = QJsonDocument::fromJson(text.toUtf8(), &json_error);
            jsonConfig=parse_doucment.object();
            return jsonConfig;
        }
    }
    configInfoSave(jsonConfig);
    return jsonConfig;
}

void MainFormOrderManagement::configInfoSave(QJsonObject &jsonConfig)
{
    QFile file(FILE_NAME_FUNCARD);

    if(file.open(QIODevice::ReadWrite | QIODevice::Text )) {
        QString text=QString(QJsonDocument(jsonConfig).toJson());
        QTextStream in(&file);
        in << text;
        in.flush();
        file.close();

        qDebug() <<"funcard config:" <<MESTools::jsonToQString(jsonConfig);
    }
}

void MainFormOrderManagement::onButtonClicked(QPushButton *button)
{
    //qDebug() << QString("Clicked: %1,%2").arg(button->objectName()).arg(button->text());
    QString btObjName=button->objectName();
    QJsonObject jsonConfig=configInfoLoad();
    QJsonObject buttonConfig=jsonConfig[btObjName].toObject();
    int CC_IsBeginEnd=0;
    {
        //        if(!MESMsgHandler::getInstance()->isConnected()){
        //            DialogToast dialog("未连接到服务器");
        //            dialog.show(1000);
        //            return ;
        //        }
    }
    /// 如果是停机功能卡,就判断是否有其它停机卡
    /// 如果是开始卡状态 就刷结束卡
    ///----显示为结束卡
    ///----显示为开始卡
    if(buttonConfig[CONFIG_IS_STOP_CARD].toBool()){
        bool haveStopCard=false;

        QStringList list=jsonConfig.keys();
        for(int i=0,j=list.size();i<j;i++){
            if(list[i]==btObjName){
                continue;
            }
            QJsonObject itemJson=jsonConfig[list[i]].toObject();
            if(itemJson[CONFIG_IS_BEGIN_END].toInt()==0){
                haveStopCard=true;
                break;
            }
        }

        if(haveStopCard==true){
            DialogToast dialog("有其它停机卡!");
            dialog.show(2000);
            return;
        }

        if(buttonConfig[CONFIG_IS_BEGIN_END].toInt()==0){
            CC_IsBeginEnd=1;  //end
        } else {
            CC_IsBeginEnd=0; //start
        }
        buttonConfig.insert(CONFIG_IS_BEGIN_END,CC_IsBeginEnd);
    }

    QString cardID="";
    //刷卡

    {
        DialogSwingCard dialog;
        QString btName=button->text();
        dialog.setTitleText(btName);
        if (dialog.exec() == QDialog::Rejected)
            return;
        //依据功能按钮寻找配置信息
        if(dialog.CardID.length()==0){
            DialogToast dialog("没有刷卡!");
            dialog.show(2000);
            return;
        }
        cardID=dialog.CardID;
    }
    {

        QString msg;
        QString DispatchNo=MESDispatchOrder::onGetMainDispatchNo();
        BrushCard bushCard;

        bushCard.CC_DispatchNo=DispatchNo;
        bushCard.CC_CardID =cardID;
        bushCard.CC_CardType=buttonConfig["CardType"].toInt();
        bushCard.CC_CardData=MESTools::GetCurrentDateTimeStr();
        bushCard.CC_IsBeginEnd=CC_IsBeginEnd;
        bushCard.Name=button->text();

        QJsonObject json=bushCard.toJsonObject();
        if(MESMsgHandler::getInstance()->submitBrushCard(json,msg) ){
            DialogToast dialog("保存成功!"+msg);
            dialog.show(2000);
            { ///save to file
                if(CC_IsBeginEnd==0)
                    buttonConfig.insert("StartTime",MESTools::GetCurrentDateTimeStr());
                if(CC_IsBeginEnd==1)
                    buttonConfig.insert("StopTime",MESTools::GetCurrentDateTimeStr());

                jsonConfig.insert(btObjName,buttonConfig);

                configInfoSave(jsonConfig);  //save to config file
            }
        } else {
            DialogToast dialog(msg);
            dialog.show(2000);
        }
    }
}

void MainFormOrderManagement::onShowAllFunButton()
{
    QPushButton *buttons[] = {
        ui->pushButton_1,ui->pushButton_2, ui->pushButton_3,ui->pushButton_4, ui->pushButton_5,ui->pushButton_6, ui->pushButton_7,
        ui->pushButton_8,ui->pushButton_9,  ui->pushButton_10,  ui->pushButton_11,ui->pushButton_12, ui->pushButton_13
    } ;

    QJsonObject jsonConfig=configInfoLoad();
    bool haveStopCardFlag=false;
    for(int i=0;i<13;i++){
        QString objectName=buttons[i]->objectName();
        QJsonObject itemJson = jsonConfig[ objectName ].toObject();

        if(itemJson[CONFIG_IS_STOP_CARD].toBool() ){
            buttons[i]->setFocusPolicy(Qt::NoFocus);
        }
        buttons[i]->setText(itemJson["Name"].toString());
        int IsBeginEnd=itemJson[CONFIG_IS_BEGIN_END].toInt();

        if(IsBeginEnd==0){
            buttons[i]->setStyleSheet("border-radius: 0px; background-color: rgb(215, 255, 171);"
                                      "color: rgb(76, 103, 255); font: 22px;");
            haveStopCardFlag=true;
        }
        if(IsBeginEnd==1){
            buttons[i]->setStyleSheet("background-color: rgb(73, 81, 93);"
                                      "color: rgb(255, 255, 255);"
                                      " border-radius: 0px;"
                                      "font: 22px;");
        }
    }
    Application::getInstance()->haveStopCardFlag=haveStopCardFlag;
}

void MainFormOrderManagement::doClicked(QWidget *widget)
{
    onButtonClicked((QPushButton*)widget);
    onShowAllFunButton();
}

void MainFormOrderManagement::onMainDispatchOrderUpdate()
{
    onShowMainDispatchOrder();
}

void MainFormOrderManagement::onShowMainDispatchOrder()
{
    QString MO_DispatchNo=MESDispatchOrder::onGetMainDispatchNo();  //先搜索主单
    int orderItemSize=MESDispatchOrder::onOrderItemSize(MO_DispatchNo);

    {
        QJsonObject json;
        QString sql=QString("SELECT * FROM %1 WHERE MO_DispatchNo='%2' order by %3 limit %4,1")
                .arg(TABLE_DORDER_ITEM)
                .arg(MO_DispatchNo)
                .arg(TABLE_FIELD_CREATED_TIME)
                .arg(orderItemIndex);

        sqlite::SQLiteBaseHelper::getInstance().selectOne(sql,json);

        ui->lineEdit_MO_DispatchNo->setText(json["MO_DispatchNo"].toString());
        ui->lineEdit_PCS_ItemNo->setText(json["PCS_ItemNO"].toString());
        ui->lineEdit_PCS_ItemName->setText(json["PCS_ItemName"].toString());
        ui->lineEdit_PCS_DispatchQty->setText(json["PCS_DispatchQty"].toString());
        ui->lineEdit_PCS_BadQty->setText(json["PCS_BadQty"].toString());
        ui->lineEdit_PCS_SocketNum1->setText(json["PCS_SocketNum1"].toString());

        int sockNum1=json["PCS_SocketNum1"].toInt();
        int moTotalNum=MESDispatchOrder::onGetDispatchNoMoTotalNum(MO_DispatchNo);
        if(sockNum1==0)
            sockNum1=1;

        ui->lineEdit_Count->setText(QString("%1").arg(sockNum1*moTotalNum));

        QString text="";
        if(orderItemSize>0){
            text=QString("%1/%2").arg(orderItemIndex+1).arg(orderItemSize);
        } else
            text="0/0";

        ui->label_TitlePageNo->setText(text);
    }
}

void MainFormOrderManagement::on_pushButton_PrevItem_clicked()
{
    //QString MO_DispatchNo=MESDispatchOrder::onGetMainDispatchNo();
    //int orderItemSize=MESDispatchOrder::onOrderItemSize(MO_DispatchNo);
    if(orderItemIndex>0)
        orderItemIndex--;
    onShowMainDispatchOrder();
}

void MainFormOrderManagement::on_pushButton_NextItem_clicked()
{
    QString MO_DispatchNo=MESDispatchOrder::onGetMainDispatchNo();
    int orderItemSize=MESDispatchOrder::onOrderItemSize(MO_DispatchNo);
    if(orderItemIndex+1<orderItemSize)
        orderItemIndex++;
    onShowMainDispatchOrder();
}

void MainFormOrderManagement::on_pushButton_report_clicked()
{
    QString msg;
    QString DispatchNo=MESDispatchOrder::onGetMainDispatchNo();
    QJsonArray array;
    MESDispatchOrder::onGetDispatchNoBaseInfos(array);

    BrushCard bushCard;

    bushCard.CC_DispatchNo=DispatchNo;
    bushCard.CC_CardID = "上报工单";
    bushCard.CC_CardType= 0xFF;
    bushCard.CC_CardData=MESTools::GetCurrentDateTimeStr();
    bushCard.CC_IsBeginEnd=0;
    bushCard.Name="上报工单";

    QJsonObject json=bushCard.toJsonObject();
    json.insert("BindDataArray",array);
    if(MESMsgHandler::getInstance()->submitBrushCard(json,msg) ){
        DialogToast dialog("成功!"+msg);
        dialog.show(2000);
    } else {
        DialogToast dialog(msg);
        dialog.show(2000);
    }
}
