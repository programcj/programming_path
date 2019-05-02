#include "mesfiledown.h"
#include "mesnet.h"
#include "../Public/public.h"
#include "../application.h"

MESFileDown::MESFileDown(const QString &name, const QByteArray &ver,
		QObject *parent) :
		QObject(parent)
{
	this->name = name;
	this->ver = ver;
	file = 0;
	downLen = 0;
	FileDownFlag = false;
	OffsetAddr = 0;
	FileSize = 0;
}
MESFileDown::~MESFileDown()
{
	if (file != 0)
		delete file;
}

//开始下载文件
bool MESFileDown::start()
{
	file = new QFile(AppInfo::getPath_DownCache() + name); //下载缓存路径
	if (file->open(QIODevice::WriteOnly))
	{
		connect(MESNet::getInstance(),
				SIGNAL(sig_MESFileDownResponse(MESFileDownResponse)), this,
				SLOT(sig_MESFileDownResponse(MESFileDownResponse)));

		connect(MESNet::getInstance(),
				SIGNAL(
						sig_MESFileDownStreamResponse(MESFileDownStreamResponse&)),
				this,
				SLOT(
						sig_MESFileDownStreamResponse(MESFileDownStreamResponse&)));
		OffsetAddr = 0; // file->size();
		FileSize = 0;
		logInfo() << "启动下载:" << this->name << ",off:" << OffsetAddr << endl;
		MESNet::getInstance()->sendRequestFileDownStart(name, ver);
		return true;
	}
	else
	{
		//文件己经打开,不能下载
		delete this;
	}
	return false;
}

//启动文件下载响应
void MESFileDown::sig_MESFileDownResponse(MESFileDownResponse response)
{
	//    0x00：允许下载程序
	//    0x01：软件版本号相同，拒绝下载
	//    0x02：FLASH空间太小，不足够存放当前应用程序
	//    0x03：服务器忙，请稍后再下载
	//    0x04：其它未知错误
	MESFileDownStreamRequest request(this->name, OffsetAddr, 1024);

	FileSize = response.FDAT_LastFileSize;
	if (FileSize <= OffsetAddr)
		OffsetAddr = 0;

	if (FileSize - OffsetAddr > 1024)  //是否 小于1024则以剩下的大小来下载
		downLen = 1024;
	else
		downLen = FileSize - OffsetAddr;

	request.FDAT_OffsetAddr = OffsetAddr; //起始偏移地址
	request.FDAT_PackSize = downLen; //本次需要传输的包大小

	switch (response.Result)
	{
	case MESPack::FDAT_RESULT_FILE_DOWN_ALLOW:
	case MESPack::FDAT_RESULT_FILE_DOWN_SPACE_SMALL:

        logDebug() << "-下载:" << this->name << " 文件大小:" << FileSize << ",开始地址:"
                << OffsetAddr << ",下载长度:" << downLen;
		MESNet::getInstance()->sendRequestFileDownStream(request);
		break;
	default:
		//其它 不能下载的要求
		logErr()<<"不能下载"<<response.Result<<endl;
		delete this;
		break;
	}
}

//文件内容保存-下载
void MESFileDown::sig_MESFileDownStreamResponse(
		MESFileDownStreamResponse &response)
{
	//    0x00：文件数据
	//    0x01：该地址非法
	//    0x02：包大小非法
	//    0x03：其它未知错误
	MESFileDownStreamRequest request(this->name, OffsetAddr, 1024);
	if (file == 0 ||response.Result==MESPack::FDAT_RESULT_FAIL_CLOSE)
	{
		delete this;
		return; //网络断开
	}
    if (response.Result == MESPack::FDAT_RESULT_FILE_DOWN_DATA ||
        response.Result == MESPack::FDAT_CHECK_SUM_ERR )
    {
        logDebug() << "写入数据:" << response.data.length() <<endl;

		file->write(response.data);
		OffsetAddr += downLen;

		if (FileSize - OffsetAddr > 1024) //是否 小于1024则以剩下的大小来下载
			downLen = 1024;
		else
			downLen = FileSize - OffsetAddr;

		request.FDAT_OffsetAddr = OffsetAddr; //起始偏移地址
		request.FDAT_PackSize = downLen; //本次需要传输的包大小

        logDebug() << "--下载:" << this->name << " 文件大小:" << FileSize << ",开始地址:"
                << OffsetAddr << ",下载长度:" << downLen;

		if (downLen != 0)
			MESNet::getInstance()->sendRequestFileDownStream(request);
		else
		{
			file->flush();
			file->close();
			//判断文件是否升级成功
			//文件是否存在
			//文件大小是否为0
			//把文件复制到配置目录
			{
				QString downFilePath = AppInfo::getPath_DownCache() + name;
				QString configPath = AppInfo::getPath_Config() ;
				if (name != "App_ChiefMES.linux")
					configPath+=name.toLower();
				else
					configPath+=name;

				QFile::remove(configPath);
				QFile::copy(downFilePath, configPath);
				QFile::remove(downFilePath);
			}
			FileDownFlag = true;
			logInfo("文件下载成功");

			//发送其它协议,文件升级成功
			OtherSetInfo::UpgradeFile info;
			OtherSetInfo myOrder;
			info.FileName = name;
			info.Result = (int) 0x08;
			myOrder.setUpgradeFile(info);
			Notebook("文件升级成功", myOrder).insert();
			//判断是否是本程序文件
            if (name == "App_ChiefMES.linux")
			{
				//发送重启指令
				if (IpcMessage::sendMsgRunNewApp())
				{

					Application::instance()->MESExit();
				}
            }
            else
            {
                //CSCPAction::GetInstance().OnConfigDownSucess(name);
                //发送重启指令 --直接重启
                if (IpcMessage::sendMsgRestartApp())
                {
                    Application::instance()->MESExit();
                }
            }
			delete this;
		}
	}
	else
	{
        logErr()<<"文件下载-出错:"<<response.Result;
		delete this;
	}
}
