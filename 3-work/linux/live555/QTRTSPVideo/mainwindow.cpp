#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QtNetwork/QHostInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags()&~Qt::WindowMaximizeButtonHint);    // 禁止最大化按钮

    QString timeString=QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss");
    statusTimeLabel = new QLabel(this);
    statusTimeLabel->setText(timeString);
    ui->statusBar->addWidget(statusTimeLabel);
    startTimer(1);

    QString localHostName = QHostInfo::localHostName();
    QHostInfo info = QHostInfo::fromName(localHostName);

    foreach(QHostAddress address,info.addresses())
    {
        if(address.protocol() == QAbstractSocket::IPv4Protocol)
        {
            ui->listLocalIP->addItem(address.toString());
        }
    }

    connect(QRTSPServer::getInstance(), &QRTSPServer::rtspServerStart, this,&MainWindow::rtspServerStart);
    connect(QRTSPServer::getInstance(), SIGNAL(rtspServerClose()),this,SLOT(rtspServerClose()));
    connect(QRTSPServer::getInstance(), SIGNAL(rtspClientConnect(QString)),this,SLOT(rtspClientConnect(QString)));
    connect(QRTSPServer::getInstance(), SIGNAL(rtspClientClose(QString)),this,SLOT(rtspClientClose(QString)));
    connect(QRTSPServer::getInstance(), SIGNAL(rtspClientDescribe(QString,QString)), this, SLOT(rtspClientDescribe(QString,QString)));

    ui->tableRTSPInfo->horizontalHeader()->setStretchLastSection(true);

    ui->tableRTSPInfo->clearContents();
    ui->listClientInfo->clear();

}

MainWindow::~MainWindow()
{
    QRTSPServer::getInstance()->disconnect();
    delete ui;
}

void MainWindow::timerEvent(QTimerEvent *e)
{
    QString timeString=QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss");

    statusTimeLabel->setText(timeString);

    QString info=QString("CPU:%1%\n").arg(JZLXSApplication::getInstance()->getSystemInfo()->cpuUse);
    info.append("内存").append(QString(JZLXSApplication::getInstance()->getSystemInfo()->memoryInfo));
    ui->textEdit->setText(info);
}

void MainWindow::rtspServerStart(const QString &urlPrefix)
{
    ui->textRtspPrefix->setText(urlPrefix);

    QDir dir(QRTSPServer::getInstance()->getVideosDir());
    if(!dir.exists())
        return;

    QStringList filters;
    filters<<QString("*.264");
    filters<<QString("*.h264");
    filters<<QString("*.H264");
    filters<<QString("*.mp4");
    filters<<QString("*.265");
    filters<<QString("*.h265");
    filters<<QString("*.H265");
    dir.setFilter(QDir::Files | QDir::NoSymLinks); //设置类型过滤器，只为文件格式
    dir.setNameFilters(filters);
    //QFileInfoList list=dir.entryInfoList(filters,QDir::Files| QDir::NoSymLinks);

    int i=0;
    ui->tableRTSPInfo->clearContents();

    mRTSPClientInfoMap.clear();

    foreach(QFileInfo mfi ,dir.entryInfoList())
    {
        if(mfi.isFile())
        {
            QString rtspUrl=QString("%1%2").arg(urlPrefix).arg(mfi.fileName());
            QMediaFileInfo info(mfi.filePath());
            QString infoStr=QString("%1x%2").arg(info.getWidth()).arg(info.getHeight());

            ui->tableRTSPInfo->setItem(i,0,new QTableWidgetItem(rtspUrl));
            ui->tableRTSPInfo->setItem(i,1,new QTableWidgetItem(infoStr));

            infoStr=info.getVideoInfo(QMediaFileInfo::VIDEO_BIT_RATE);

            ui->tableRTSPInfo->setItem(i,2,new QTableWidgetItem(infoStr));
            ui->tableRTSPInfo->setItem(i,3,new QTableWidgetItem("0"));
            i++;
        }
    }

    ui->tableRTSPInfo->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tableRTSPInfo->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
}

void MainWindow::rtspServerClose()
{
    ui->tableRTSPInfo->clearContents();
    ui->listClientInfo->clear();
    mRTSPClientInfoMap.clear();
}

void MainWindow::rtspClientConnect(const QString ip)
{
    QListWidgetItem* item = new QListWidgetItem;
    item->setText(ip);
    ui->listClientInfo->addItem(item);
    ui->lineClientSize->setText(QString("%1").arg(ui->listClientInfo->count()));
}

void MainWindow::rtspClientClose(const QString ip)
{
    if(0 == ui->listClientInfo->count())    //获取items数量，当QListWidget中没有item时返回
        return;

    QList<QListWidgetItem*> list=ui->listClientInfo->findItems(ip, Qt::MatchStartsWith);

    qDebug()<<"listClients clear->"<<ip << " size:"<<list.length();
    for(int i=0;i<list.length();i++){
        delete list.at(i);
    }

    ui->lineClientSize->setText(QString("%1").arg(ui->listClientInfo->count()));

    if(mRTSPClientInfoMap.contains(ip)) {
        QString urlSuffix=mRTSPClientInfoMap[ip];
        for(int i=0;i<ui->tableRTSPInfo->rowCount();i++)
        {
            if(ui->tableRTSPInfo->item(i,0)==NULL)
                continue;

            if(ui->tableRTSPInfo->item(i,0)->text().endsWith(urlSuffix)){
                QString v=ui->tableRTSPInfo->item(i,3)->text();
                if(v.toInt()>0){
                    v=QString::number(v.toInt()-1);
                    ui->tableRTSPInfo->item(i,3)->setText(v);
                }
            }
        }
    }
}

void MainWindow::rtspClientDescribe(const QString &clientIP, const QString &urlSuffix)
{
    qDebug()<<clientIP<<"|"<<urlSuffix;

    mRTSPClientInfoMap.insert(clientIP,urlSuffix);

    for(int i=0;i<ui->tableRTSPInfo->rowCount();i++)
    {
        if(ui->tableRTSPInfo->item(i,0)==NULL)
            continue;

        if(ui->tableRTSPInfo->item(i,0)->text().endsWith(urlSuffix)){
            QString v=ui->tableRTSPInfo->item(i,3)->text();
            v=QString::number(v.toInt()+1);
            ui->tableRTSPInfo->item(i,3)->setText(v);
        }
    }
}

void MainWindow::on_btSelectPath_clicked()
{
    QFileDialog dialog;
    QString dirString;
    dialog.setWindowTitle("选择视频文件目录");
    dialog.setFileMode(QFileDialog::DirectoryOnly);
    if( QDialog::Accepted != dialog.exec())
        return ;
    dirString= dialog.selectedFiles().at(0);
    qDebug()<<"设定RTSP目录:"<<dirString;
    QRTSPServer::getInstance()->setVideosDir(dirString);
    ui->labelMideaFilePath->setText(QString("RTSP目录：%1").arg(dirString));
}

void MainWindow::on_btRTSPRun_clicked()
{
    //QMessageBox::information(NULL, "Title", "checked true", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    //JZLXSApplication::getInstance()->quit(); //类似qApp->quit();

    if(ui->btRTSPRun->isChecked()){
        ui->btRTSPRun->setText("关闭");
        QString dirString=QRTSPServer::getInstance()->getVideosDir();

        if(dirString.length()==0){
            QMessageBox::information(NULL,"提示","未设定视频文件目录");
            return;
        }
        ui->labelMideaFilePath->setText(QString("RTSP目录：%1").arg(dirString));
        ui->textRtspPrefix->setText("");
        QRTSPServer::getInstance()->startRtspServer();
    } else {
        ui->btRTSPRun->setText("开启");
        ui->textRtspPrefix->setText("");
        QRTSPServer::getInstance()->stopRtspServer();
        ui->textRtspPrefix->setText("");
    }
}

void MainWindow::on_tableRTSPInfo_customContextMenuRequested(const QPoint &pos)
{
    QMenu *menu = new QMenu(ui->tableRTSPInfo);
    //menu->addAction(new QAction(tr("&Copy"), menu));
    QAction *copy=menu->addAction(tr("&Copy"));
    //QAction *videoInfo=menu->addAction(tr("&Video Info"));

    //menu->move(cursor().pos()); //让菜单显示的位置在鼠标的坐标上
    QAction *exec=menu->exec(QCursor::pos());
    if(exec==copy)
    {
        QList<QTableWidgetItem*> items = ui->tableRTSPInfo->selectedItems();
        int count = items.count();
        QString copyStr;
        for(int i = 0; i < count; i++)
        {
            int row = ui->tableRTSPInfo->row(items.at(i));
            QTableWidgetItem *item = items.at(i);
            QString name = item->text(); //获取内容
            copyStr.append(name);

            if(i!=count-1)
                copyStr.append("\n");
        }

        QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
        QString originalText = clipboard->text();	     //获取剪贴板上文本信息
        clipboard->setText(copyStr);		             //设置剪贴板内容
        if(originalText.length()>10){
            originalText=originalText.mid(0,10);
        }
        qDebug()<<count<<" 剪切板内容"<<originalText<<" ...修改:"<<copyStr;
    }
    //if(exec==videoInfo){
    //ui->tableRTSPInfo->selectedIndexes();
    //}
    delete menu;
}

void MainWindow::on_listLocalIP_itemDoubleClicked(QListWidgetItem *item)
{
    qDebug()<<item->text();

    for(int i=0;i<ui->tableRTSPInfo->rowCount();i++)
    {
        //rtsp://192.168.100.15:8880/
        if(ui->tableRTSPInfo->item(i,0)==NULL)
            continue;
        QString url=ui->tableRTSPInfo->item(i,0)->text();
        int index=url.indexOf(':', 7);

        url=QString("rtsp://").append(item->text()).append(url.mid(index, url.length()));
        ui->tableRTSPInfo->item(i,0)->setText(url);
    }
}
