/**
* Application 类
* 公共资源都放这里, QT App启动就直接在这里
*
* qApp->quit(); 替换成 JZLXSApplication::getInstance()->quit();
*
* 在 live555的BasicUsageEnvironment类中有执行WSAStartup来初始化windows的socket库
*
*  cj 2019/3/25
*/
#ifndef JZLXSAPPLICATION_H
#define JZLXSAPPLICATION_H

#include <QApplication>
#include "qmediafileinfo.h"
#include "qrtspserver.h"
#include "qsysteminfo.h"

/**
 * @brief The JZLXSApplication class
 *
 *
 */
class JZLXSApplication : public QApplication
{
    static JZLXSApplication *instance;

    QRTSPServer *rtspser;
    QSystemInfo *systemInfo;

public:
    static JZLXSApplication *getInstance();

    JZLXSApplication(int &argc, char **argv);
    ~JZLXSApplication();

    QSystemInfo *getSystemInfo() {
        return systemInfo;
    }

};


#endif // JZLXSAPPLICATION_H
