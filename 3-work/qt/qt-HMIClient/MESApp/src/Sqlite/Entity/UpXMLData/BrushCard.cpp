/*
 * BrushCard.cpp
 *
 *  Created on: 2015年1月26日
 *      Author: cj
 */

#include "BrushCard.h"

namespace entity
{

BrushCard::BrushCard()
{
	//id = 0;			// primary key AutoIncrement
	DispatchNo = ""; //	20	ASC	派工单号
	DispatchPrior = ""; //	30	ASC	派工项次
	ProcCode = ""; //	20	ASC	工序代码
	StaCode = ""; //	10	ASC	站别代码
	CardID = ""; //	10	HEX	卡号
	cardType = 0; //	1	HEX	刷卡原因编号
	CardDate = ""; //	6	HEX	刷卡数据产生时间
	IsBeginEnd = 0; //	1	HEX	刷卡开始或结束标记，0表示开始，1表示结束。
	//CarryDataLen = 0; //	2	HEX	刷卡携带数据长度（N）
	CarryData = ""; //	N	HEX	刷卡携带数据内容
}

} /* namespace mes_protocol */
