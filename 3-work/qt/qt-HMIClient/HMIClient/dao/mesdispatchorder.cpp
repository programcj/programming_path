#include "sqlitebasehelper.h"
#include "mesdispatchorder.h"

static QString TB_CREATE_ORDER="CREATE TABLE IF NOT EXISTS Mes_DispatchOrder ("
                               "MO_DispatchNo varchar(33) PRIMARY KEY,"//	20	string	派工单号
        "MO_ProcCode varchar(30),"//	20	string	工序代码
        "MO_ProcName varchar(55),"//	50	string	工序名称
        "MO_StaCode varchar(30),"//	10	string	站别代码
        "MO_StaName varchar(30),"//	20	string	站别名称（工作中心）
        "MO_StandCycle integer,"//	4	DWORD	标准周期，毫秒
        "MO_TotalNum integer,"//	4	DWORD	模次,生产次数（原啤数）
        "MO_MultiNum integer,"//	1	integer	总件数N（0< N <= 100）
        "MO_BadReasonNum integer,"//	1	integer	次品原因类型总数M（0< M <= 100）
        "MainOrderFlag integer,"//主单序号
        "No1 integer,"
        "No2 integer," //
        "Str1 varchar,"
        "Str2 varchar,"//预留的
        "CreatedTime TIMESTAMP NOT NULL DEFAULT (DATETIME ('now', 'localtime'))"\
        ")";

static QString TB_CREATE_ORDER_BODY ="CREATE TABLE IF NOT EXISTS Mes_DispatchOrder_Item  ("
         "ID integer PRIMARY KEY autoincrement,"  //ID
        "MO_DispatchNo varchar(33),"    //派工单号
        "PCS_MO varchar(30),"   //	20	string	工单号（件）
        "PCS_ItemNO varchar(25)," //	20	string	产品编号
        "PCS_ItemName varchar(55)," //	50	string	产品描述
        "PCS_MouldNo varchar(25)," //	20	string	模具编号
        "PCS_DispatchQty integer," //	4	DWORD	派工数量
        "PCS_SocketNum1 integer," //	1	integer	该件模穴数
        "PCS_BadQty integer," //	4	DWORD	次品总数
        "PCS_BadData varchar," //   每件产品的次品数据（对应次品原因的数据）
        "TotalOkNum integer,"       //良品数
        "CurClassProductNo integer,"    //本班产品数
        "CurClassBadNo integer,"    //本班次品数
        "CreatedTime TIMESTAMP NOT NULL DEFAULT (DATETIME ('now', 'localtime'))"
        ")";
/*******************
//"	ID integer PRIMARY KEY autoincrement,"
//"	MO_DispatchNo varchar(30),"//派工单号
//"	PCS_MO varchar(25),"//工单号
//"	PCS_ItemNO varchar(25),"//产品编号
//"	PCS_ItemName varchar(55),"//产品描述
//"	PCS_MouldNo varchar(25),"//模具编号
//"	PCS_DispatchQty integer,"//派工数量
//"	PCS_SocketNum1 integer,"//模具可用模穴数
//"	PCS_SocketNum2 integer,"//产品可用模穴数
//"	PCS_FitMachineMin integer,"//模具适应机型吨位最小值
//"	PCS_FitMachineMax integer,"//模具适应机型吨位最大值
//"	PCS_BadQty integer,	"//次品总数
//"	PCS_BadData varchar,"//每件产品的次品数据（对应次品原因的数据）
//"	PCS_AdjNum integer,"//调机数
//" TotalOkNum integer,"//良品数
//"	CurClassProductNo integer,"//本班产品数
//"	CurClassBadNo integer,"//本班次品数
//"	CurClassPolishNo integer,"//打磨数
//"	CurClassInspectNo integer,"//寻机数
//"	ADJDefCount integer,"	//调机次品数
//"ADJOKNum integer,"//调机良品数
//"ADJEmptyMoldNum integer," //调机空模数
//"ADJDefList varchar,"  //调机中的次品列表
**************************/

MESDispatchOrder *MESDispatchOrder::instance=NULL;

MESDispatchOrder::MESDispatchOrder(QObject *parent):
    QObject(parent)
{
    instance=this;
    MO_DispatchNo=MESDispatchOrder::onGetMainDispatchNo();
}

MESDispatchOrder *MESDispatchOrder::getInstance()
{
    return instance;
}

void MESDispatchOrder::emitMainOrderUpdate()
{
    qDebug() << "emit -> main order update.......";
    MO_DispatchNo=MESDispatchOrder::onGetMainDispatchNo();

    emit sigMainOrderUpdate();
}

void MESDispatchOrder::onCreate(sqlite::SQLiteBaseHelper *dbHelp)
{
    dbHelp->exec(TB_CREATE_ORDER);
    dbHelp->exec(TB_CREATE_ORDER_BODY);
}

long MESDispatchOrder::onOrderSize()
{
    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    QString sql=QString("SELECT COUNT(*) FROM %1").arg(TABLE_DISPATCH_ORDER);
    if (query.exec(sql))
    {
        if(query.next()){
            //qDebug() << QString("count(*)=%1").arg(query.value(0).toString());
            return query.value(0).toInt();
        }
    }
    return 0;
}

int MESDispatchOrder::onOrderItemSize(QString MO_DispatchNo)
{
    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    QString sql=QString("SELECT COUNT(*) FROM %1 WHERE MO_DispatchNo='%2' ").arg(TABLE_DORDER_ITEM).arg(MO_DispatchNo);
    if (query.exec(sql))
    {
        if(query.next()){
            //qDebug() << QString("count(*)=%1").arg(query.value(0).toString());
            return query.value(0).toInt();
        }
    }
    return 0;
}

int MESDispatchOrder::onCountPK(QString no)
{
    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    QString sql=QString("SELECT COUNT(*) FROM %1 WHERE MO_DispatchNo=?").arg(TABLE_DISPATCH_ORDER);
    query.prepare(sql);
    query.addBindValue(no);
    if(query.exec()){
        if(query.next()){
            return query.value(0).toInt();
        }
    } else {
        qDebug()<< QString("Err:%1").arg(sql);
    }
    return 0;
}

QString MESDispatchOrder::onGetMainDispatchNo()
{
    QString MO_DispatchNo="";
    QString sql=QString("SELECT MO_DispatchNo FROM %1 ORDER BY %2 LIMIT 0,1").arg(TABLE_DISPATCH_ORDER).arg(TABLE_FIELD_CREATED_TIME);
    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    if(query.exec(sql)) {
        if(query.next()){
            MO_DispatchNo=query.value(0).toString();
        }
    } else {
        qDebug()<< QString("Err:%1").arg(sql);
    }

    return MO_DispatchNo;
}

bool MESDispatchOrder::onGetDispatchNoBaseInfos(QJsonArray &array)
{
    //
    //    SELECT a.MO_DispatchNo,b.PCS_ItemNo from
    //    Mes_DispatchOrder as a
    //     LEFT JOIN
    //     Mes_DispatchOrder_Item as b ON a.MO_DispatchNo=b.MO_DispatchNo
    //     ORDER BY a.CreatedTime desc
    QString sql="SELECT a.MO_DispatchNo,b.PCS_ItemNo,a.CreatedTime from "
                "Mes_DispatchOrder as a "
                "LEFT JOIN "
                "Mes_DispatchOrder_Item as b ON a.MO_DispatchNo=b.MO_DispatchNo "
                "ORDER BY a.CreatedTime desc";
    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    if( query.exec(sql)) {
        while (query.next())
        {
            int size = query.record().count();
            QSqlRecord record = query.record();
            QJsonObject json;

            for(int i=0;i<size;i++){
                QVariant value = query.value(i);
                if (value.isNull())
                    continue;
                ////// qDebug() <<QString("%1=%2>>").arg(record.fieldName(i)).arg(value.toString());
                json.insert(record.fieldName(i), value.toString());
            }
            array.append(json);
        }
        return true;
    } else {
        qDebug() << "Error:"<<sql<<query.lastError().text();
    }
    return false;
}

/**
 * 获取这个派工单的模次
 * @brief onGetDispatchNoMoTotalnum
 * @param dispatchNo
 * @return
 */
int MESDispatchOrder::onGetDispatchNoMoTotalNum(QString &MO_DispatchNo)
{
    QString sql="SELECT MO_TotalNum FROM Mes_DispatchOrder WHERE MO_DispatchNo=?";
    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    query.prepare(sql);
    query.addBindValue(MO_DispatchNo);
    if(query.exec()){
        if(query.next()){
            return query.value(0).toInt();
        }
        return 0;
    } else {
        qDebug()<< QString("Err:%1, %2").arg(sql).arg(MO_DispatchNo);
    }
    return 0;
}

int MESDispatchOrder::onGetDispatchNoItemCount(QString MO_DispatchNo, QString PCS_ItemNO)
{
    int totalNum=MESDispatchOrder::onGetDispatchNoMoTotalNum(MO_DispatchNo);
    int count=0;
    {
        QSqlQuery queryItem(sqlite::SQLiteBaseHelper::getInstance().getDB()); //....该件模穴数,派工数量
        queryItem.prepare("SELECT ID,PCS_ItemNO,PCS_SocketNum1,PCS_DispatchQty FROM Mes_DispatchOrder_Item WHERE MO_DispatchNo=? AND PCS_ItemNO=? LIMIT 0,1");
        queryItem.addBindValue(MO_DispatchNo);
        queryItem.addBindValue(PCS_ItemNO);
        if(queryItem.exec()){
            if(queryItem.next()){
                int PCS_SocketNum1= queryItem.value(2).toInt();
                //int PCS_DispatchQty=queryItem.value(3).toInt();
                PCS_SocketNum1=PCS_SocketNum1==0?1:PCS_SocketNum1;
                count= PCS_SocketNum1*totalNum;
            }
        }
    }
    return count;
}

bool MESDispatchOrder::onCheckProductedNumberEnd(QString &MO_DispatchNo)
{
    int MoTotalNum=MESDispatchOrder::onGetDispatchNoMoTotalNum(MO_DispatchNo);
    QSqlQuery queryItem(sqlite::SQLiteBaseHelper::getInstance().getDB());//....该件模穴数,派工数量
    queryItem.prepare("SELECT ID,PCS_ItemNO,PCS_SocketNum1,PCS_DispatchQty FROM Mes_DispatchOrder_Item WHERE MO_DispatchNo=? LIMIT 0,1");
    queryItem.addBindValue(MO_DispatchNo);

    if(queryItem.exec()){
        if(queryItem.next()){
            int PCS_SocketNum1= queryItem.value(2).toInt();
            int PCS_DispatchQty=queryItem.value(3).toInt();
            if(PCS_SocketNum1==0)
                PCS_SocketNum1=1;

            if(PCS_DispatchQty<=PCS_SocketNum1*MoTotalNum)
                return true;
        }
    }
    return false;
}

bool MESDispatchOrder::onAddMOTotalNum(QString &MO_DispatchNo, int value)
{
    bool rc=false;
    if(onCountPK(MO_DispatchNo)==0)
        return rc;
    {
        QString sql="UPDATE Mes_DispatchOrder  SET MO_TotalNum=MO_TotalNum+? WHERE MO_DispatchNo=?";
        QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
        query.prepare(sql);
        query.addBindValue(value);
        query.addBindValue(MO_DispatchNo);

        rc=sqlite::SQLiteBaseHelper::getInstance().exec(query, sql, __FILE__, __FUNCTION__, __LINE__);
    }
    QList<int> adds;
    QList<int> ids;
    {
        //每一件的显示呢? 也须要增加,,   模具可用模穴数,派工数量
        //增加本班生产数 ?
        QSqlQuery queryItem(sqlite::SQLiteBaseHelper::getInstance().getDB());
        queryItem.prepare("SELECT ID,PCS_ItemNO,PCS_SocketNum1,PCS_DispatchQty FROM Mes_DispatchOrder_Item WHERE MO_DispatchNo=?");
        queryItem.addBindValue(MO_DispatchNo);
        if(queryItem.exec()){
            while(queryItem.next()){
                ids.append(queryItem.value(0).toInt());
                int PCS_SocketNum1=queryItem.value(2).toInt();
                if(PCS_SocketNum1==0)
                    PCS_SocketNum1=1;
                adds.append(value*PCS_SocketNum1);
            }
        }
    }
    //    {
    //        for(int i=0,j=ids.size();i<j;i++){
    //            QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    //            QString sql=QString("UPDATE "TABLE_DORDER_ITEM" SET PCS_DispatchQty=PCS_DispatchQty+%1 WHERE ID=%2")
    //                    .arg(adds[i])
    //                    .arg(ids[i]);
    //            query.exec(sql);
    //        }
    //    }
    MESDispatchOrder::getInstance()->emitMainOrderUpdate();
    return rc;
}

void MESDispatchOrder::onGetDispatchNoItems(const QString MO_DispatchNo, QJsonArray &jsonArray)
{
    QString sql=QString("SELECT * FROM %1 WHERE MO_DispatchNo='%2' ORDER BY %3")
            .arg(TABLE_DORDER_ITEM)
            .arg(MO_DispatchNo)
            .arg(TABLE_FIELD_CREATED_TIME);

    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());

    qDebug() <<sql;

    if (query.exec(sql))
    {
        while (query.next())
        {
            int size = query.record().count();
            QSqlRecord record = query.record();
            QJsonObject json;

            for(int i=0;i<size;i++){
                QVariant value = query.value(i);
                if (value.isNull())
                    continue;
                //////
                qDebug() <<QString("%1=%2>>").arg(record.fieldName(i)).arg(value.toString());

                json.insert(record.fieldName(i), value.toString());
            }
            jsonArray.append(json);
        }
    }
}

void MESDispatchOrder::onGetDispatchNoItem(const QString &MO_DispatchNo,const QString &PCS_ItemNO, QJsonObject &json)
{
    QString sql=QString("SELECT * FROM %1 WHERE MO_DispatchNo='%2' AND PCS_ItemNO='%3' ORDER BY %4 LIMIT 0,1")
            .arg(TABLE_DORDER_ITEM)
            .arg(MO_DispatchNo)
            .arg(PCS_ItemNO)
            .arg(TABLE_FIELD_CREATED_TIME);

    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());

    qDebug() <<sql;

    if (query.exec(sql))
    {
        if (query.next())
        {
            int size = query.record().count();
            QSqlRecord record = query.record();

            for(int i=0;i<size;i++){
                QVariant value = query.value(i);
                if (value.isNull())
                    continue;
                json.insert(record.fieldName(i), value.toString());
            }
        }
    }
}

void MESDispatchOrder::onDispatchItemSaveBadData(const QString &MO_DispatchNo, const QString &PCS_ItemNO,const QJsonArray &badDataArray)
{
    QString PCS_BadDataString=QString(QJsonDocument(badDataArray).toJson());

    QString sql="UPDATE Mes_DispatchOrder_Item SET PCS_BadData=? WHERE MO_DispatchNo=? and PCS_ItemNO=?";
    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    query.prepare(sql);
    query.addBindValue(PCS_BadDataString);
    query.addBindValue(MO_DispatchNo);
    query.addBindValue(PCS_ItemNO);

    sqlite::SQLiteBaseHelper::getInstance().exec(query, sql, __FILE__, __FUNCTION__, __LINE__);
}

void MESDispatchOrder::onDispatchItemAddBadQty(const QString &MO_DispatchNo, const QString &PCS_ItemNo, int value)
{
    QString sql="UPDATE Mes_DispatchOrder_Item SET PCS_BadQty=PCS_BadQty+? WHERE MO_DispatchNo=? and PCS_ItemNO=?";
    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    query.prepare(sql);
    query.addBindValue(value);
    query.addBindValue(MO_DispatchNo);
    query.addBindValue(PCS_ItemNo);
    sqlite::SQLiteBaseHelper::getInstance().exec(query, sql, __FILE__, __FUNCTION__, __LINE__);

    instance->emitMainOrderUpdate();
}

void MESDispatchOrder::onDeleteDispatchNo(const QString &MO_DispatchNo)
{
    //QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    QString sql=QString("DELETE FROM %1 WHERE MO_DispatchNo='%2'").arg(TABLE_DISPATCH_ORDER).arg(MO_DispatchNo);
    sqlite::SQLiteBaseHelper::getInstance().exec(sql);

    sql=QString("DELETE FROM %1 WHERE MO_DispatchNo='%2'").arg(TABLE_DORDER_ITEM).arg(MO_DispatchNo);
    sqlite::SQLiteBaseHelper::getInstance().exec(sql);
}

/**
 * 添加工单，这里一般是从服务器派单过来
 *
 * @brief MESDispatchOrder::onSubimtOrder
 * @param json
 * @return
 */
bool MESDispatchOrder::onSubimtOrder(QJsonObject &json)
{
    ///1,先保存派工单信息
    /// 2,保存工单的每个派工单中的工单信息
    int FDAT_DataType=json.take("FDAT_DataType").toInt();
    QJsonObject FDAT_DataJson=json["FDAT_Data"].toObject();

    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    QString sql = "REPLACE INTO Mes_DispatchOrder  ("
                                                     "MO_DispatchNo,"//	20	string	派工单号
            "MO_ProcCode,"//	20	string	工序代码
            "MO_ProcName,"//	50	string	工序名称
            "MO_StaCode,"//	10	string	站别代码
            "MO_StaName,"//	20	string	站别名称（工作中心）
            "MO_StandCycle,"//	4	DWORD	标准周期，毫秒
            "MO_TotalNum,"//	4	DWORD	模次,生产次数（原啤数）
            "MO_MultiNum,"//	1	integer	总件数N（0< N <= 100）
            "MO_BadReasonNum,"//	1	integer	次品原因总数M（0< M <= 100）
            "MainOrderFlag"	//主单序号
            ") VALUES (?,?,?,?,?,?,?,?,?,?)";

    query.prepare(sql);

    QString mo_dispatchNo=FDAT_DataJson["MO_DispatchNo"].toString();
    query.bindValue(0,mo_dispatchNo);
    query.bindValue(1,FDAT_DataJson["MO_ProcCode"].toString());
    query.bindValue(2,FDAT_DataJson["MO_ProcName"].toString());
    query.bindValue(3,FDAT_DataJson["MO_StaCode"].toString());
    query.bindValue(4,FDAT_DataJson["MO_StaName"].toString());
    query.bindValue(5,FDAT_DataJson["MO_StandCycle"].toInt());
    query.bindValue(6,FDAT_DataJson["MO_TotalNum"].toInt());
    query.bindValue(7,FDAT_DataJson["MO_MultiNum"].toInt());
    query.bindValue(8,FDAT_DataJson["MO_BadReasonNum"].toInt());
    query.bindValue(9,FDAT_DataJson["FDAT_DataType"].toInt());

    if(mo_dispatchNo.length()==0){
        qDebug() << "工单添加失败,工单编号为空";
        return false;
    }

    if (false == sqlite::SQLiteBaseHelper::getInstance().exec(query, sql, __FILE__, __FUNCTION__, __LINE__)){
        qDebug() << "工单添加失败";
        return false;
    }

    qDebug() << "工单添加成功";

    QJsonArray MO_DataArray = FDAT_DataJson["MO_Data"].toArray(); //N件产品的主单数据

    //遍历MO_Data--插入子表中
    for(int i=0,j=MO_DataArray.size();i<j;i++){
        QJsonObject moDataItemJson=MO_DataArray.at(i).toObject();
        //         moDataItemJson.take("PCS_MO");//	20	string	工单号
        //         moDataItemJson.take("PCS_ItemNO");//	20	string	产品编号
        //         moDataItemJson.take("PCS_ItemName");//	50	string	产品描述
        //         moDataItemJson.take("PCS_MouldNo");//	20	string	模具编号
        //         moDataItemJson.take("PCS_DispatchQty");//	4	DWORD	派工数量
        //         moDataItemJson.take("PCS_SocketNum1");//	1	integer	该件模穴数
        //         moDataItemJson.take("PCS_BadQty");//	4	DWORD	次品总数

        int id=-1; //65535?
        {
            QString str=QString(QJsonDocument(moDataItemJson).toJson());
            qDebug() <<"item json:"<<str;
        }
        QString PCS_ItemNO=moDataItemJson["PCS_ItemNO"].toString();
        {
            QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
            query.prepare("SELECT ID from Mes_DispatchOrder_Item where MO_DispatchNo=? and PCS_ItemNO=?"); //派工单号   产品编号
            query.addBindValue(mo_dispatchNo);
            query.addBindValue(PCS_ItemNO);
            if (sqlite::SQLiteBaseHelper::getInstance().exec(query, "select id body",__FILE__,
                                                             __FUNCTION__, __LINE__))
            {
                if (query.next())
                { //找到ID
                    id=query.value(0).toInt();
                }
            }
        }
        QJsonArray PCS_BadData= moDataItemJson["BadData"].toArray();//	M *4	Array	每件产品的次品数据（对应次品原因的数据）（见1.2.5.3）
        QString PCS_BadDataString=QString(QJsonDocument(PCS_BadData).toJson());

        QString sql="REPLACE INTO Mes_DispatchOrder_Item (";
        if(id!=-1)
            sql.append("ID,");
        sql.append(
                    "MO_DispatchNo,"    //派工单号
                    "PCS_MO,"   //	20	string	工单号（件）
                    "PCS_ItemNO," //	20	string	产品编号
                    "PCS_ItemName," //	50	string	产品描述
                    "PCS_MouldNo," //	20	string	模具编号
                    "PCS_DispatchQty," //	4	DWORD	派工数量
                    "PCS_SocketNum1," //	1	integer	该件模穴数
                    "PCS_BadQty,"
                    "PCS_BadData) values ("); //	4	DWORD	次品总数,
        if(id!=-1)
            sql.append("?,");
        sql.append("?,?,?,?,?,?,?,?,?)");
        {
            QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
            query.prepare(sql);
            int index=0;
            if(id!=-1)
                query.bindValue(index++,id);
            query.bindValue(index++,mo_dispatchNo);
            query.bindValue(index++,moDataItemJson["PCS_MO"].toString());
            query.bindValue(index++,moDataItemJson["PCS_ItemNO"].toString());
            query.bindValue(index++,moDataItemJson["PCS_ItemName"].toString());
            query.bindValue(index++,moDataItemJson["PCS_MouldNo"].toString());
            query.bindValue(index++,moDataItemJson["PCS_DispatchQty"].toInt());
            query.bindValue(index++,moDataItemJson["PCS_SocketNum1"].toInt());
            query.bindValue(index++,moDataItemJson["PCS_BadQty"].toInt());
            query.bindValue(index++,PCS_BadDataString);

            qDebug() << QString("子表:%1").arg(moDataItemJson["PCS_MO"].toString());

            if (sqlite::SQLiteBaseHelper::getInstance().exec(query,sql,__FILE__,
                                                             __FUNCTION__, __LINE__))
            {

            } else {
                qDebug() << "子表添加失败"; //不考虑这个情况
            }
        }
    }

    if(FDAT_DataType==0 || MESDispatchOrder::onOrderSize()==1){
        MESDispatchOrder::getInstance()->emitMainOrderUpdate();
    }
    return true;
}
