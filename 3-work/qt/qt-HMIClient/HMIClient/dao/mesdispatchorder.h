#ifndef MESDISPATCHORDER_H
#define MESDISPATCHORDER_H

#include <QString>
#include <QtCore>
#include <QObject>

#include "../public.h"
#include "sqlitebasehelper.h"

class MESDispatchOrder : public QObject
{
    static MESDispatchOrder *instance;

    QString MO_DispatchNo;

    Q_OBJECT
public:
    explicit MESDispatchOrder(QObject *parent = 0);

    static MESDispatchOrder *getInstance();

Q_SIGNALS:
    void sigMainOrderUpdate();

public:

    QString getDispatchNo() { return MO_DispatchNo; }

    void setDispatchNo(const QString &dispatchNo) { MO_DispatchNo=dispatchNo; }

    void emitMainOrderUpdate();

    /**
     * @brief onCreate
     * @param dbHelp
     */
    static void onCreate(sqlite::SQLiteBaseHelper *dbHelp);

    //public static void onInsert();
    static long onOrderSize();

    static int onOrderItemSize(QString MO_DispatchNo);

    static int onCountPK(QString no);

    static QString onGetMainDispatchNo();

    static bool onGetDispatchNoBaseInfos(QJsonArray &array);

    /**
     * 获取这个派工单的模次
     * @brief onGetDispatchNoMoTotalnum
     * @param dispatchNo
     * @return
     */
    static int onGetDispatchNoMoTotalNum(QString &MO_DispatchNo);

    /**
     * 获取这个派工单哪一件生产数
     * @brief onGetDispatchNoItemCount
     * @param MO_DispatchNo  派工单
     * @param PCS_ItemNO  件
     * @return
     */
    static int onGetDispatchNoItemCount(QString MO_DispatchNo,QString PCS_ItemNO);

    static bool onCheckProductedNumberEnd(QString &MO_DispatchNo);

    /**
     * 增加模次
     * @brief onAddMOTotalNum
     * @param MO_DispatchNo
     * @param value
     * @return
     */
    static bool onAddMOTotalNum(QString &MO_DispatchNo, int value);

    /**
     * 获取此工单的产品列表
     * @brief onGetDispatchNoItems
     * @param MO_DispatchNo
     * @param json
     */
    static void onGetDispatchNoItems(const  QString MO_DispatchNo, QJsonArray &jsonArray);

    static void onGetDispatchNoItem(const QString &MO_DispatchNo, const QString &PCS_ItemNO, QJsonObject &json);

    static void onDispatchItemSaveBadData(const QString &MO_DispatchNo, const QString &PCS_ItemNO,const QJsonArray &badDataArray);

    /**
     * 增加此派工单的哪一件的次品总数
     *
     * @brief onDispatchItemAddBadQty
     * @param MO_DispatchNo
     * @param PCS_ItemNo
     * @param value
     */
    static void onDispatchItemAddBadQty(const QString &MO_DispatchNo, const QString &PCS_ItemNo,int value);

    /**
     * @brief onDeleteDispatchNo
     * @param MO_DispatchNo
     */
    static void onDeleteDispatchNo(const QString &MO_DispatchNo);

    /**
     * 提交一个工单
     * @brief onSubimtOrder
     * @param json
     * @return
     */
    static bool onSubimtOrder(QJsonObject &json);
};

#endif // MESDISPATCHORDER_H
