/*
 * ButtonStatus.h
 *
 *  Created on: 2015年5月20日
 *      Author: cj
 */

#ifndef BUTTONSTATUS_H_
#define BUTTONSTATUS_H_

#include "../AppConfig/AppInfo.h"
#include "../Unit/Tool.h"

using namespace unit;

//按钮状态
class ButtonStatus: public QObject, public AbsPropertyBase
{
Q_OBJECT
private:
	virtual void onBindProperty();
	int showStatus; //显示状态 1为结束 0为开始
	QString chanageTime; //按钮改变时间
	QString bindText; //绑待的数据 ,
public:
	QString name; //名字
	int funIndex; //功能序号

	ButtonStatus();
	ButtonStatus(const QString &name, int index, int status);

	//是否显示为开始卡
	bool isShowStart()
	{
		return showStatus == 0;
	}

	//是否显示为结束卡
	bool isShowEnd()
	{
		return showStatus == 1;
	}

	//设定为开始刷卡
	void setShowStartCard()
	{
		chanageTime = Tool::GetCurrentDateTimeStr();
		showStatus = 0;
		emit onStatChange(this);
	}

	//设定为结束刷卡
	void setShowEndCard()
	{
		chanageTime = Tool::GetCurrentDateTimeStr();
		showStatus = 1;
		emit onStatChange(this);
	}

	//取得按钮改变的时间
	const QString getChanageTime()
	{
		return chanageTime;
	}

	const QString getBindText()
	{
		return bindText;
	}

	void setBindText(const QString &text)
	{
		bindText = text;
		emit onStatChange(this);
	}

	virtual const char *getClassName() const;

	//获取这个按钮的配置文件中的配置信息
	MESPDFuncCfg::SetKeyInfo* getKeyInfo();

Q_SIGNALS:
	void onStatChange(ButtonStatus *bt); //状态改变信号
};

//所有页按钮状态
//功能按钮 状态控制
class UIPageButtonStatus: public QObject
{
    Q_OBJECT
private:
    QList<ButtonStatus*> uiBtStatus; //UI按钮状态,

    QString fileSavePath;
    static UIPageButtonStatus *instance;
public:
    explicit UIPageButtonStatus(QObject *parent = 0);

    ~UIPageButtonStatus();

    static UIPageButtonStatus *GetInstance();

    /**
     * @brief XMLFileReadUIBtStatus
     *  从XML配置文件中读取出配置按钮信息
     */
    void XMLFileReadUIBtStatus();

    //保存所有的配置按钮信息
    void XMLFileWriteUIBtStatus();

    //改变按钮状态
    void changeButtonStatus(ButtonStatus *btStatus);

    //通过按钮配置信息获取按钮状态
    ButtonStatus *getUIBtStatus(MESPDFuncCfg::SetKeyInfo &keyInfo);

    //通过功能序号获取按钮状态
    ButtonStatus *getUIBtStatus(int funIndex);

    //判断是否有停机卡
    bool isStopCard();

    //除了这个卡btStatus 是否还有其它停机卡
    //条件： 这个卡己 显示开始 + 有停机功能
    bool isOtherStopCard(ButtonStatus *btStatus);

Q_SIGNALS:
    void onSigFunButtonChange(ButtonStatus* bt);

private slots:
    void onButtonStatChang(ButtonStatus* bt); //按钮状态改变
};

#endif /* BUTTONSTATUS_H_ */
