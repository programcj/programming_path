#include "Order.h"
#include "../SQLiteBaseHelper.h"
#include "../SQLiteProductedHelper.h"

namespace entity
{

BEGIN_COLUMNS_MAP(tb_orderBoy,OrderBoy)
COLUMNS_INT	( ID, &OrderBoy::setId )
	COLUMNS_STR( MO_DispatchNo, &OrderBoy::setMoDispatchNo )
	COLUMNS_STR( PCS_MO, &OrderBoy::setPcsMo )
	COLUMNS_STR( PCS_ItemNO, &OrderBoy::setPcsItemNo )
	COLUMNS_STR( PCS_ItemName, &OrderBoy::setPcsItemName )
	COLUMNS_STR( PCS_MouldNo, &OrderBoy::setPcsMouldNo )
	COLUMNS_INT( PCS_DispatchQty, &OrderBoy::setPcsDispatchQty )
	COLUMNS_INT( PCS_SocketNum1, &OrderBoy::setPcsSocketNum1 )
	COLUMNS_INT( PCS_SocketNum2, &OrderBoy::setPcsSocketNum2 )
	COLUMNS_INT( PCS_FitMachineMin, &OrderBoy::setPcsFitMachineMin )
	COLUMNS_INT( PCS_FitMachineMax, &OrderBoy::setPcsFitMachineMax )
	COLUMNS_INT( PCS_BadQty, &OrderBoy::setPcsBadQty )
	COLUMNS_STR( PCS_BadData, &OrderBoy::setPcsBadData )
	COLUMNS_INT( PCS_AdjNum, &OrderBoy::setPcsAdjNum )
	COLUMNS_INT( TotalOkNum, &OrderBoy::setTotalOkNum )
	COLUMNS_INT( CurClassProductNo, &OrderBoy::setCurClassProductNo )
	COLUMNS_INT( CurClassBadNo, &OrderBoy::setCurClassBadNo )
	COLUMNS_INT( CurClassPolishNo, &OrderBoy::setCurClassPolishNo )
	COLUMNS_INT( CurClassInspectNo, &OrderBoy::setCurClassInspectNo )
	COLUMNS_INT( ADJDefCount, &OrderBoy::setAdjDefCount )
	COLUMNS_INT( ADJOKNum, &OrderBoy::setADJOKNum )
	COLUMNS_INT( ADJEmptyMoldNum, &OrderBoy::setADJEmptyMoldNum )
	COLUMNS_STR( ADJDefList, &OrderBoy::setADJDefList )
	COLUMNS_STR( Str2, &OrderBoy::setStr2 )
	END_COLUMNS_MAP()

OrderBoy::OrderBoy()
{
	setId(0);
	setPcsDispatchQty(0);

	PCS_DispatchQty = 0;
	PCS_SocketNum1 = 0;
	PCS_SocketNum2 = 0;
	PCS_FitMachineMin = 0;
	PCS_FitMachineMax = 0;
	PCS_BadQty = 0;
	PCS_BadData.clear();

	PCS_AdjNum = 0;
	TotalOkNum = 0;
	CurClassProductNo = 0;
	CurClassBadNo = 0;
	CurClassPolishNo = 0;
	CurClassInspectNo = 0;
	ADJDefCount = 0;
	ADJOKNum = 0;
	ADJEmptyMoldNum = 0;
	ADJDefList = "";
	str2 = "";
}

bool OrderBoy::subimt()
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare(
			"SELECT ID from tb_orderBoy where MO_DispatchNo=? and PCS_ItemNO=?"); //派工单号   产品编号
	query.addBindValue(MO_DispatchNo);
	query.addBindValue(PCS_ItemNO);
	if (sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__))
	{
		if (query.next())
		{
			setId(query.value(0).toInt());
		}
		else
		{
			setId(-1);
		}
	}
	else
	{
		setId(-1);
		//qDebug() << query.lastError().text();
	}

	if (-1 != getId())
		query.prepare("REPLACE INTO tb_orderBoy ("
				"ID,               "
				"MO_DispatchNo,    "
				"PCS_MO,           "
				"PCS_ItemNO,       "
				"PCS_ItemName,     "
				"PCS_MouldNo,      "
				"PCS_DispatchQty,  "
				"PCS_SocketNum1,   "
				"PCS_SocketNum2,   "
				"PCS_FitMachineMin,"
				"PCS_FitMachineMax,"
				"PCS_BadQty,       "
				"PCS_BadData,      "
				"PCS_AdjNum,       "
				"TotalOkNum,       "
				"CurClassProductNo,"
				"CurClassBadNo,    "
                "CurClassPolishNo,         "
                "CurClassInspectNo,        "
                "ADJOKNum,              "
                "ADJEmptyMoldNum,              "
                "ADJDefList,             "
				"str2) values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
	else
		query.prepare("REPLACE INTO tb_orderBoy ("
				"MO_DispatchNo,    "
				"PCS_MO,           "
				"PCS_ItemNO,       "
				"PCS_ItemName,     "
				"PCS_MouldNo,      "
				"PCS_DispatchQty,  "
				"PCS_SocketNum1,   "
				"PCS_SocketNum2,   "
				"PCS_FitMachineMin,"
				"PCS_FitMachineMax,"
				"PCS_BadQty,       "
				"PCS_BadData,      "
				"PCS_AdjNum,       "
				"TotalOkNum,       "
				"CurClassProductNo,"
				"CurClassBadNo,    "
                "CurClassPolishNo,  "
                "CurClassInspectNo, "
                "ADJOKNum,          "
                "ADJEmptyMoldNum,   "
                "ADJDefList,        "
				"str2) values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
	if (-1 != getId())
		query.addBindValue(ID);

	query.addBindValue(MO_DispatchNo);
	query.addBindValue(PCS_MO);
	query.addBindValue(PCS_ItemNO);
	query.addBindValue(PCS_ItemName);
	query.addBindValue(PCS_MouldNo);
	query.addBindValue(PCS_DispatchQty);
	query.addBindValue(PCS_SocketNum1);
	query.addBindValue(PCS_SocketNum2);
	query.addBindValue(PCS_FitMachineMin);
	query.addBindValue(PCS_FitMachineMax);
	query.addBindValue(PCS_BadQty);
	{   //query.addBindValue(PCS_BadData);
		QString _str;
		for (int i = 0; i < PCS_BadData.length(); i++)
		{
			QString v;
			_str += v.sprintf("%d", PCS_BadData[i]);
			if (i < PCS_BadData.length() - 1)
				_str += ",";
		}
		query.addBindValue(_str);
	}
	query.addBindValue(PCS_AdjNum);
	query.addBindValue(TotalOkNum);
	query.addBindValue(CurClassProductNo);
	query.addBindValue(CurClassBadNo);
	query.addBindValue(CurClassPolishNo);
	query.addBindValue(CurClassInspectNo);
	query.addBindValue(ADJOKNum);
	query.addBindValue(ADJEmptyMoldNum);
	query.addBindValue(ADJDefList);
	query.addBindValue(str2);
	if (sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__))
		return true;
	return false;
}

bool OrderBoy::remove()
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("delete * from OrderBoy where ID=? ");
	query.bindValue(0, ID);
	return sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__);
}

QSqlQuery OrderBoy::getQSqlQuery()
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	return query;
}

sqlite::SQLiteOpenHelper &OrderBoy::getSQLiteOpenHelper()
{
	return sqlite::SQLiteBaseHelper::getInstance();
}

bool OrderBoy::OnAddADJOKNum(int value)
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    query.prepare("UPDATE tb_orderBoy SET ADJOKNum=ADJOKNum+? WHERE ID=?");
	query.addBindValue(value);
	query.addBindValue(getId());
	setADJOKNum(getADJOKNum() + value);
	return getSQLiteOpenHelper().exec(query);
}

bool OrderBoy::OnAddADJEmptyMoldNum(int value)
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    query.prepare("UPDATE tb_orderBoy set ADJEmptyMoldNum=ADJEmptyMoldNum+? where ID=?");
	query.addBindValue(value);
	query.addBindValue(getId());
    setADJEmptyMoldNum(getADJEmptyMoldNum() + value);
	return getSQLiteOpenHelper().exec(query);
}
//产品可用模穴数
bool OrderBoy::OnUpdatePcsSocketNum2(int value)
{
	setPcsSocketNum2(value);
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("UPDATE tb_orderBoy set PCS_SocketNum2=? where ID=?");
	query.addBindValue(value);
	query.addBindValue(getId());
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}

//更新每件次品数
bool OrderBoy::OnUpdatePCSBadData(const QList<int>& list)
{
	this->PCS_BadData.clear();
	this->PCS_BadData.append(list);
	QString _str;
	for (int i = 0; i < PCS_BadData.length(); i++)
	{
		QString v;
		_str += v.sprintf("%d", PCS_BadData[i]);
		if (i < PCS_BadData.length() - 1)
			_str += ",";
	}

	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("UPDATE tb_orderBoy set PCS_BadData=? where ID=?");
	query.addBindValue(_str);
	query.addBindValue(getId());
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}
//预留的 调机良品数
bool OrderBoy::OnUpdateADJOKNum(int value)
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    query.prepare("UPDATE tb_orderBoy set ADJOKNum=? where id=?");
	query.addBindValue(value);
	query.addBindValue(getId());
	setADJOKNum(value);
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}
//预留的 调机空模数
bool OrderBoy::OnUpdateADJEmptyMoldNum(int value)
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    query.prepare("UPDATE tb_orderBoy set ADJEmptyMoldNum=? where id=?");
	query.addBindValue(value);
	query.addBindValue(getId());
	setADJEmptyMoldNum(value);
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}

bool OrderBoy::OnUpdateADJDefList(const QString& str)
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    query.prepare("UPDATE tb_orderBoy set ADJDefList=? where id=?");
	query.addBindValue(str);
	query.addBindValue(getId());
	setADJDefList(str);
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}
//更新调机数
bool OrderBoy::OnUpdateADJDefCount(int value)
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("UPDATE tb_orderBoy set ADJDefCount=? where id=?");
	query.addBindValue(value);
	query.addBindValue(getId());
	setAdjDefCount(value);
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}

//更新调机数
bool OrderBoy::OnAddADJDefCount(int value)
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare(
			"UPDATE tb_orderBoy set ADJDefCount=ADJDefCount+? where id=?");
	query.addBindValue(value);
	query.addBindValue(getId());
	setAdjDefCount(getAdjDefCount() + value);
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}

//添加 次品数
bool OrderBoy::OnAddPcsBadQty(int value)
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("UPDATE tb_orderBoy set PCS_BadQty=PCS_BadQty+? where id=?");
	query.addBindValue(value);
	query.addBindValue(getId());
	setPcsBadQty(getPcsBadQty() + value);
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}
//更新良品数
bool OrderBoy::OnUpdateTotalOkNum(int value)
{
	TotalOkNum = value;
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("UPDATE tb_orderBoy set TotalOkNum=? where id=?");
	query.addBindValue(TotalOkNum);
	query.addBindValue(getId());
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}
//换班
bool OrderBoy::OnReplaceClass()
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare(
            "UPDATE tb_orderBoy set CurClassProductNo=0,CurClassBadNo=0,CurClassPolishNo=0,CurClassInspectNo=0 where id=?");
	query.addBindValue(getId());
	CurClassProductNo = 0;
	CurClassBadNo = 0;
	CurClassPolishNo = 0;
	CurClassInspectNo = 0;
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}

//增加本班生产数
bool OrderBoy::OnAddCurClassProductNo(int value)
{
	CurClassProductNo += value; //* PCS_SocketNum2
	TotalOkNum += value; //良品须增加

	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare(
			"UPDATE tb_orderBoy set TotalOkNum=?,CurClassProductNo=CurClassProductNo+? where id=?");
	query.addBindValue(TotalOkNum);
	query.addBindValue(value);
	query.addBindValue(getId());
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}

//增加本班次品数
bool OrderBoy::OnAddCurClassBadNo(int value)
{
	CurClassBadNo += value;
	TotalOkNum -= value; //良品须减去

	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare(
			"UPDATE tb_orderBoy set TotalOkNum=?,CurClassBadNo=CurClassBadNo+? where id=?");
	query.addBindValue(TotalOkNum);
	query.addBindValue(value);
	query.addBindValue(getId());
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}
//增加本班打磨数
bool OrderBoy::OnAddCurClassPolishNo(int value)
{
	CurClassPolishNo += value;
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    query.prepare("UPDATE tb_orderBoy set CurClassPolishNo=CurClassPolishNo+? where id=?");
	query.addBindValue(value);
	query.addBindValue(getId());
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}
//增加本班寻机数
bool OrderBoy::OnAddCurClassInspectNo(int value)
{
	CurClassInspectNo += value;
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    query.prepare("UPDATE tb_orderBoy set CurClassInspectNo=CurClassInspectNo+? where id=?");
	query.addBindValue(value);
	query.addBindValue(getId());
	return getSQLiteOpenHelper().exec(query, __FILE__, __FUNCTION__, __LINE__);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *
 */
///
BEGIN_COLUMNS_MAP(tb_order, Order)
COLUMNS_STR	( MO_DispatchNo, &Order::setMoDispatchNo)
	COLUMNS_STR( MO_DispatchPrior, &Order::setMoDispatchPrior)
	COLUMNS_STR( MO_ProcCode, &Order::setMoProcCode)
	COLUMNS_STR( MO_ProcName, &Order::setMoProcName)
	COLUMNS_STR( MO_StaCode, &Order::setMoStaCode)
	COLUMNS_STR( MO_StaName, &Order::setMoStaName)
	COLUMNS_INT( MO_StandCycle, &Order::setMoStandCycle)
	COLUMNS_INT( MO_TotalNum, &Order::setMoTotalNum)
	COLUMNS_INT( MO_MultiNum, &Order::setMoMultiNum)
	COLUMNS_INT( MO_BadTypeNo, &Order::setMoBadTypeNo)
	COLUMNS_INT( MO_BadReasonNum, &Order::setMoBadReasonNum)
	COLUMNS_INT( MainOrder_FLAG, &Order::setMainOrderFlag)
    //COLUMNS_INT( ProductionNo, &Order::setProductionNo)
	END_COLUMNS_MAP()
////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////

Order::Order()
{
	init();
}

void Order::init()
{
	MO_DispatchNo = "";	//派工单号 PRIMARY KEY
	MO_DispatchPrior = "";  //派工项次
	MO_ProcCode = "";	//工序代码
	MO_ProcName = "";	//工序名称
	MO_StaCode = "";	//站别代码
	MO_StaName = "";	//站别名称（工作中心）
	MO_StandCycle = 0;
	MO_TotalNum = 0;
	MO_MultiNum = 0;
	MO_BadTypeNo = 0;
	MO_BadReasonNum = 0;
	MainOrder_FLAG = 0;
    //ProductionNo = 0;
	orderBoyList.clear();
}

bool Order::subimt()
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());

	query.prepare("REPLACE INTO tb_order ("
			"MO_DispatchNo,     "
			"MO_DispatchPrior,  "
			"MO_ProcCode,       "
			"MO_ProcName,       "
			"MO_StaCode,        "
			"MO_StaName,        "
			"MO_StandCycle,     "
			"MO_TotalNum,       "
			"MO_MultiNum,       "
			"MO_BadTypeNo,      "
			"MO_BadReasonNum,   "
            "MainOrder_FLAG     "
            ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?)");
	query.bindValue(0, MO_DispatchNo);
	query.bindValue(1, MO_DispatchPrior);
	query.bindValue(2, MO_ProcCode);
	query.bindValue(3, MO_ProcName);
	query.bindValue(4, MO_StaCode);
	query.bindValue(5, MO_StaName);
	query.bindValue(6, MO_StandCycle);
	query.bindValue(7, MO_TotalNum);
	query.bindValue(8, MO_MultiNum);
	query.bindValue(9, MO_BadTypeNo);
	query.bindValue(10, MO_BadReasonNum);
    query.bindValue(11, MainOrder_FLAG);

	if (sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__))
	{
		for (int i = 0; i < orderBoyList.size(); i++)
		{
			OrderBoy boy = orderBoyList.at(i);
			boy.setMoDispatchNo(MO_DispatchNo);
			boy.subimt(); //这里可以 加判断,如果返回false, 就删掉这个派工单, 再返回false
		}

		return true;
	}
	return false;
}

bool Order::remove()
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());

	query.prepare("delete from tb_order where MO_DispatchNo=?");
	query.bindValue(0, MO_DispatchNo);
	if (sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__))
	{
		query.prepare("delete from tb_orderBoy where MO_DispatchNo=?");
		query.bindValue(0, MO_DispatchNo);
		//删除子表,工单每件
		sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
				__FUNCTION__, __LINE__);
		//删除原料
		query.prepare("delete from tb_Material where DispatchNo=?");
		query.bindValue(0, MO_DispatchNo);
		sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
				__FUNCTION__, __LINE__);
		return true;
	}
	return false;
}

/**
 * Function Name: getBoy
 *   Para:
 * Return Values: QList<OrderBoy> &
 * Comments:查询工单的产品列表
 */
QList<OrderBoy> &Order::getBoy()
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare(
			"select * from tb_orderBoy where MO_DispatchNo='"
					+ getMoDispatchNo() + "' order by id asc");

	if (sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__,
			__LINE__))
		while (query.next())
		{
			OrderBoy boy;
			boy.read(query);
			orderBoyList.append(boy);
		}
	return orderBoyList;
}

/**
 * Function Name: query
 *   Para:
 *   	Order &order :目标变量
 *   	int index 工单序号
 * Return Values: bool 成功,失败
 * Comments:查询工单,序号为 index的单
 *  主单是从1开始
 */
bool Order::query(Order& order, int mainOrderFlag)
{
	QString selectArg;
	selectArg.sprintf("MainOrder_FLAG=%d", mainOrderFlag);
	order.init();
	return sqlite::SQLiteBaseHelper::getInstance().selectItem(order, "*",
			selectArg, "MainOrder_FLAG");
}

bool Order::query(Order& order, const QString& DispatchNo)
{
	QString selectArg;
	selectArg = "MO_DispatchNo='";
	selectArg += DispatchNo;
	selectArg += "'";

	return sqlite::SQLiteBaseHelper::getInstance().selectItem(order, "*",
			selectArg, "MainOrder_FLAG");
}

Order Order::query(const QString& DispatchNo)
{
	QString selectArg;
	Order order;
	selectArg = "MO_DispatchNo='";
	selectArg += DispatchNo;
	selectArg += "'";

	sqlite::SQLiteBaseHelper::getInstance().selectItem(order, "*", selectArg,
			"MainOrder_FLAG");
	return order;
}

/**
 * Function Name: queryAll
 *   Para:
 *   	QList<Order> &list : 目标list
 * Return Values: bool 成功,失败
 * Comments:查询所有的工单,按序号排序的
 */
bool Order::queryAll(QList<Order>& list)
{
	//以MainOrder_FLAG 排序从小到大
	return sqlite::SQLiteBaseHelper::getInstance().selectList(list,
			Order::GetThisTableName(), "*", "", "MainOrder_FLAG");
}

/**
 * 重新排序
 * 工单序号从1开始
 */
bool Order::sort()
{
	//重新排序
	QList<Order> list;
	if (Order::queryAll(list))
	{
		QList<Order>::iterator iter = list.begin();
		for (int i = 1; iter != list.end(); iter++)
		{
			iter->setMainOrderFlag(i++);
			iter->subimt();
		}
	}
	return true;
}

/**
 * 更改工单的排序号
 */
bool Order::UpdateMainOrderFlag(const QString &DispatchNo, int index)
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("update tb_order set MainOrder_FLAG=? where MO_DispatchNo=?");
	query.addBindValue(index);
	query.addBindValue(DispatchNo);

	if (sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__))
	{
		return true;
	}
	return false;
}
/**
 * Function Name: count
 * Return Values: int 工单条数
 * Comments:查询所有的工单的条数
 */
int Order::count()
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("SELECT count(*) from tb_order");
	if (sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__))
	{
		if (query.next())
		{
			return query.value(0).toInt();
		}
	}
	return 0;
}

int Order::countMODispatchNo(QString MO_DispatchNo)
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("SELECT count(*) FROM tb_order WHERE MO_DispatchNo=?");
	query.addBindValue(MO_DispatchNo);
	if (sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__))
	{
		if (query.next())
		{
			return query.value(0).toInt();
		}
	}
	return 0;
}
//增加模次
bool Order::OnAddMOTotalNum(int value)
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare(
            "UPDATE tb_order SET MO_TotalNum=MO_TotalNum+? WHERE MO_DispatchNo=?");
	query.addBindValue(value);
	query.addBindValue(MO_DispatchNo);
    setMoTotalNum(getMoTotalNum() + value);

	if (sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__))
	{
		return true;
	}
	return false;
}

bool Order::OnUpdateStandCycle(int cycle)
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("UPDATE tb_order SET MO_StandCycle=? WHERE MO_DispatchNo=?");
	query.addBindValue(cycle);
	query.addBindValue(MO_DispatchNo);

	if (sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__))
	{
		return true;
	}
	return false;
}

}
