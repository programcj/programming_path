#ifndef WORKORDER_H
#define WORKORDER_H

#include "absentity.h"
#include <QList>
#include "../SQLiteOpenHelper.h"

namespace entity
{

class OrderBoy: public AbsEntity
{
	int ID; // integer PRIMARY KEY autoincrement,"
	QString MO_DispatchNo;	//派工单号
	QString PCS_MO;		//工单号 [CSCP]
	QString PCS_ItemNO;	//产品编号[CSCP]
	QString PCS_ItemName;	//产品描述[CSCP]
	QString PCS_MouldNo;	//模具编号[CSCP]
	int PCS_DispatchQty;	//派工数量[CSCP]
	int PCS_SocketNum1;	//模具可用模穴数,也就是  产品总模穴数[CSCP]
	int PCS_SocketNum2;	//产品可用模穴数[CSCP]
	int PCS_FitMachineMin;	//模具适应机型吨位最小值[CSCP]
	int PCS_FitMachineMax;	//模具适应机型吨位最大值[CSCP]
	int PCS_BadQty;			//次品总数,也就是次品的总个数,为每个次品数之和 [CSCP]
	QList<int> PCS_BadData;	//每件产品的次品数据（对应次品原因的数据,每一个对应一个次品原因个数）[CSCP]
	int PCS_AdjNum;			//调机数[CSCP]

	int TotalOkNum;			//良品数;  [总生产数量  = 产品模穴数* 模次 ]，良品数=总生产数量-次品数 +调机数，
							//这个良品并不是本班良品
	int CurClassProductNo;	//本班生产总数    = 本班模次* 产品模穴数
	int CurClassBadNo;		//本班次品总数
    int CurClassPolishNo;	//打磨数
    int CurClassInspectNo;	//寻机数

	int ADJDefCount;	//调机次品数

    int ADJOKNum; 		//调机良品数
    int ADJEmptyMoldNum;		//调机空模数  调机生产总数= 调机良品数+调机空模数+调机次品
    QString ADJDefList; //预留的 用于调机中的次品列表
	QString str2; //预留的

DECLARE_COLUMNS_MAP()
	;
public:
	OrderBoy();

	virtual bool subimt();
	virtual bool remove();
	QSqlQuery getQSqlQuery();
	sqlite::SQLiteOpenHelper &getSQLiteOpenHelper();

	//返回本班良品数=
	int getCurClassOKNum() const
	{
		//本班: 生产-次品-寻机数
		return CurClassProductNo-CurClassBadNo-CurClassInspectNo;
	}
	//返回本班次品率-比率=本班次品总数/本班生产总数
	float getCurClassBadRate() const
	{
		if(CurClassProductNo!=0) //0 不能做除数
			return CurClassBadNo*1.0/CurClassProductNo;
		return 0.0;
	}
	//begin get and set
	//本班次品总数
	int getCurClassBadNo() const
	{
		return CurClassBadNo;
	}

	//本班次品总数
	void setCurClassBadNo(int curClassBadNo)
	{
		CurClassBadNo = curClassBadNo;
	}
	//本班生产总数
	int getCurClassProductNo() const
	{
		return CurClassProductNo;
	}
	//本班生产总数
	void setCurClassProductNo(int curClassProductNo)
	{
		CurClassProductNo = curClassProductNo;
	}
	//本班良品数
	int getCurClassTotalOkNum(){
		return CurClassProductNo-CurClassBadNo;
	}

	int getId() const
	{
		return ID;
	}

	void setId(int id)
	{
		ID = id;
	}
	//寻机数
	int getCurClassInspectNo() const
	{
		return CurClassInspectNo;
	}
	//寻机数
	void setCurClassInspectNo(int inspectNo)
	{
		CurClassInspectNo = inspectNo;
	}
	//派工单号
	const QString& getMoDispatchNo() const
	{
		return MO_DispatchNo;
	}
	//派工单号
	void setMoDispatchNo(const QString& moDispatchNo)
	{
		MO_DispatchNo = moDispatchNo;
	}
	//调机次品数
	int getAdjDefCount() const
	{
		return ADJDefCount;
	}
	//调机次品数
	void setAdjDefCount(int adjDefCount)
	{
		ADJDefCount = adjDefCount;
	}

	//预留的 调机良品数
	int getADJOKNum() const
	{
		return ADJOKNum;
	}
	//预留的 调机良品数
	void setADJOKNum(int no1)
	{
		ADJOKNum = no1;
	}
	//预留的 调机空模数  调机生产总数= 调机良品数+调机空模数+调机次品
	int getADJEmptyMoldNum() const
	{
		return ADJEmptyMoldNum;
	}
	//预留的 调机空模数  调机生产总数= 调机良品数+调机空模数+调机次品
	void setADJEmptyMoldNum(int no2)
	{
		ADJEmptyMoldNum = no2;
	}
	//调机数
	int getPcsAdjNum() const
	{
		return PCS_AdjNum;
	}
	//调机数
	void setPcsAdjNum(int pcsAdjNum)
	{
		PCS_AdjNum = pcsAdjNum;
	}
	//注意须要判断大小哦 ！
	QList<int>& getPcsBadData()
	{
		return PCS_BadData;
	}

	void setPcsBadData(const QString& pcsBadData)
	{
		PCS_BadData.clear();
		if (pcsBadData.length() > 0)
		{
			QStringList sections = pcsBadData.split(',');
			for (int i = 0; i < sections.length(); i++)
			{
				PCS_BadData.append(sections.at(i).toInt());
			}
		}
	}
	//次品总数
	int getPcsBadQty() const
	{
		return PCS_BadQty;
	}
	//次品总数
	void setPcsBadQty(int pcsBadQty)
	{
		PCS_BadQty = pcsBadQty;
	}
	//派工数量
	int getPcsDispatchQty() const
	{
		return PCS_DispatchQty;
	}
	//派工数量
	void setPcsDispatchQty(int pcsDispatchQty)
	{
		PCS_DispatchQty = pcsDispatchQty;
	}
	//模具适应机型吨位最大值
	int getPcsFitMachineMax() const
	{
		return PCS_FitMachineMax;
	}
	//模具适应机型吨位最大值
	void setPcsFitMachineMax(int pcsFitMachineMax)
	{
		PCS_FitMachineMax = pcsFitMachineMax;
	}
	//模具适应机型吨位最小值
	int getPcsFitMachineMin() const
	{
		return PCS_FitMachineMin;
	}
	//模具适应机型吨位最小值
	void setPcsFitMachineMin(int pcsFitMachineMin)
	{
		PCS_FitMachineMin = pcsFitMachineMin;
	}
	//产品描述
	const QString& getPcsItemName() const
	{
		return PCS_ItemName;
	}
	//产品描述
	void setPcsItemName(const QString& pcsItemName)
	{
		PCS_ItemName = pcsItemName;
	}
	//产品编号
	const QString& getPcsItemNo() const
	{
		return PCS_ItemNO;
	}
	//产品编号
	void setPcsItemNo(const QString& pcsItemNo)
	{
		PCS_ItemNO = pcsItemNo;
	}
	//工单号
	const QString& getPcsMo() const
	{
		return PCS_MO;
	}
	//工单号
	void setPcsMo(const QString& pcsMo)
	{
		PCS_MO = pcsMo;
	}
	//模具编号
	const QString& getPcsMouldNo() const
	{
		return PCS_MouldNo;
	}
	//模具编号
	void setPcsMouldNo(const QString& pcsMouldNo)
	{
		PCS_MouldNo = pcsMouldNo;
	}
	//模具可用模穴数
	int getPcsSocketNum1() const
	{
		return PCS_SocketNum1;
	}
	//模具可用模穴数
	void setPcsSocketNum1(int pcsSocketNum1)
	{
		PCS_SocketNum1 = pcsSocketNum1;
	}
	//产品可用模穴数
	int getPcsSocketNum2() const
	{
		return PCS_SocketNum2;
	}
	//产品可用模穴数
	void setPcsSocketNum2(int pcsSocketNum2)
	{
		PCS_SocketNum2 = pcsSocketNum2;
	}
	//打磨数
	int getCurClassPolishNo() const
	{
		return CurClassPolishNo;
	}
	//打磨数
	void setCurClassPolishNo(int polishNo)
	{
		CurClassPolishNo = polishNo;
	}
	//预留的 用于调机中的次品列表
	const QString& getADJDefList() const
	{
		return ADJDefList;
	}
	//预留的 用于调机中的次品列表
	void setADJDefList(const QString& str1)
	{
		this->ADJDefList = str1;
	}
	//预留的
	const QString& getStr2() const
	{
		return str2;
	}
	//预留的
	void setStr2(const QString& str2)
	{
		this->str2 = str2;
	}
	//良品数
	int getTotalOkNum() const
	{
		return TotalOkNum;
	}
	//良品数 [总生产数量  = 产品模穴数* 模次 ]，良品数=总生产数量-次品数 +调机数，
	void setTotalOkNum(int totalOkNum)
	{
		TotalOkNum = totalOkNum;
	}

	//单列保存功能
    bool OnAddADJOKNum(int value); //调机良品
    bool OnAddADJEmptyMoldNum(int value); //调机空模数
	bool OnAddPcsBadQty(int value); //添加 次品数

	bool OnUpdatePcsSocketNum2(int value); //产品可用模穴数
	bool OnUpdatePCSBadData(const QList<int> &list); //更新每件次品数
    bool OnUpdateADJOKNum(int value);//预留的 调机良品数
    bool OnUpdateADJEmptyMoldNum(int value);//预留的 调机空模数
    bool OnUpdateADJDefList(const QString &str); //用于调机中的次品列表
	bool OnUpdateADJDefCount(int value);	//更新调机次品数

	bool OnUpdateTotalOkNum(int value); //更新良品数
	bool OnReplaceClass(); //换班 清空本班生产数等
	bool OnAddCurClassProductNo(int value); //增加本班生产数
	bool OnAddCurClassBadNo(int value);	//增加本班次品数
	bool OnAddCurClassPolishNo(int value);	//增加本班打磨数
	bool OnAddCurClassInspectNo(int value);	//增加本班寻机数
	bool OnAddADJDefCount(int value); //添加调机次品数
};

class Order: public AbsEntity
{
	QString MO_DispatchNo;	//派工单号 PRIMARY KEY [CSCP]
	QString MO_DispatchPrior;  //派工项次 [CSCP]
	QString MO_ProcCode;	//工序代码 [CSCP]
	QString MO_ProcName;	//工序名称 [CSCP]
	QString MO_StaCode;		//站别代码 [CSCP]
	QString MO_StaName;		//站别名称（工作中心）[CSCP]
	int MO_StandCycle;		//标准周期，毫秒 [CSCP]
	int MO_TotalNum;		//模次,生产次数（原啤数）[CSCP]
	int MO_MultiNum;		//总件数N（0< N <= 100） [CSCP]
	int MO_BadTypeNo;		//次品类型选项（配置文件中,通过这个类型 选择哪一类次品原因） [CSCP]
	int MO_BadReasonNum;	//次品原因总数M（0< M <= 100, 指 原因的个数,不是次品个数哦） [CSCP]
	int MainOrder_FLAG;		//工单的序号
    //int ProductionNo;		//模次   可以计算出[本班产品数　本班次品数]

DECLARE_COLUMNS_MAP()
	;
public:
	QList<OrderBoy> orderBoyList;

	Order();
	void init();

	virtual bool subimt();
	virtual bool remove();

	/**
	 * Function Name: getBoy
	 *   Para:
	 * Return Values: QList<OrderBoy> &
	 * Comments:查询工单的产品列表
	 */
	QList<OrderBoy> &getBoy();

	/**
	 * Function Name: query
	 *   Para:
	 *   	Order &order :目标变量
	 *   	int index 工单序号
	 * Return Values: bool 成功,失败
	 * Comments:查询工单,序号为 index的单
	 */
	static bool query(Order &order, int index);

	/**
	 * 更据派工单号 查询工单信息
	 */
	static bool query(Order &order, const QString &DispatchNo);

	/**
	 * 更据派工单号 查询工单信息
	 */
	static Order query(const QString &DispatchNo);

	/**
	 * Function Name: queryAll
	 *   Para:
	 *   	QList<Order> &list : 目标list
	 * Return Values: bool 成功,失败
	 * Comments:查询所有的工单,按序号排序的
	 */
	static bool queryAll(QList<Order> &list);

	/**
	 * 重新排序
	 */
	static bool sort();

	/**
	 * 更改工单的排序号
	 */
	static bool UpdateMainOrderFlag(const QString &DispatchNo, int index);

	/**
	 * Function Name: count
	 * Return Values: int 工单条数
	 * Comments:查询所有的工单的条数
	 */
	static int count();

	/**
	 * Function Name: countMODispatchNo
	 * Return Values: int 工单条数
	 * Comments: 判断此工单有多少个
	 */
	static int countMODispatchNo(QString MO_DispatchNo);

	//单列保存功能
    bool OnAddMOTotalNum(int value);		//增加模次
	//修改周期
	bool OnUpdateStandCycle(int cycle);

public:
	//get set
	int getMainOrderFlag() const
	{
		return MainOrder_FLAG;
	}

	void setMainOrderFlag(int mainOrderFlag)
	{
		MainOrder_FLAG = mainOrderFlag;
	}

	int getMoBadReasonNum() const
	{
		return MO_BadReasonNum;
	}

	void setMoBadReasonNum(int moBadReasonNum)
	{
		MO_BadReasonNum = moBadReasonNum;
	}
	//次品类型选项（配置文件中）
	int getMoBadTypeNo() const
	{
		return MO_BadTypeNo;
	}

	void setMoBadTypeNo(int moBadTypeNo)
	{
		MO_BadTypeNo = moBadTypeNo;
	}

	const QString& getMoDispatchNo() const
	{
		return MO_DispatchNo;
	}

	void setMoDispatchNo(const QString& moDispatchNo)
	{
		MO_DispatchNo = moDispatchNo;
	}

	const QString& getMoDispatchPrior() const
	{
		return MO_DispatchPrior;
	}

	void setMoDispatchPrior(const QString& moDispatchPrior)
	{
		MO_DispatchPrior = moDispatchPrior;
	}

	int getMoMultiNum() const
	{
		return MO_MultiNum;
	}

	void setMoMultiNum(int moMultiNum)
	{
		MO_MultiNum = moMultiNum;
	}

	const QString& getMoProcCode() const
	{
		return MO_ProcCode;
	}

	void setMoProcCode(const QString& moProcCode)
	{
		MO_ProcCode = moProcCode;
	}

	const QString& getMoProcName() const
	{
		return MO_ProcName;
	}

	void setMoProcName(const QString& moProcName)
	{
		MO_ProcName = moProcName;
	}

	const QString& getMoStaCode() const
	{
		return MO_StaCode;
	}

	void setMoStaCode(const QString& moStaCode)
	{
		MO_StaCode = moStaCode;
	}

	const QString& getMoStaName() const
	{
		return MO_StaName;
	}

	void setMoStaName(const QString& moStaName)
	{
		MO_StaName = moStaName;
	}

	int getMoStandCycle() const
	{
		return MO_StandCycle;
	}

	void setMoStandCycle(int moStandCycle)
	{
		MO_StandCycle = moStandCycle;
	}
    //模次,生产次数（原啤数）[CSCP]
    int getMoTotalNum() const
	{
		return MO_TotalNum;
	}
    //模次,生产次数（原啤数）[CSCP]
	void setMoTotalNum(int moTotalNum)
	{
		MO_TotalNum = moTotalNum;
	}

	const QList<OrderBoy>& getOrderBoyList() const
	{
		return orderBoyList;
	}

	void setOrderBoyList(const QList<OrderBoy>& orderBoyList)
	{
		this->orderBoyList = orderBoyList;
	}

public:

};

}
#endif // WORKORDER_H
