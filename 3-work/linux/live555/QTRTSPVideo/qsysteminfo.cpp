#include "qsysteminfo.h"


#ifdef _WIN32
#include <windows.h>
//https://blog.csdn.net/luo_sen/article/details/87983270
class SystemInfo {
    int nCPUPercentage = 0;
    FILETIME idleTime;
    FILETIME kernelTime;
    FILETIME userTime;
    FILETIME pre_idleTime;
    FILETIME pre_kernelTime;
    FILETIME pre_userTime;

public:

    void init(){
        int res;
        res = GetSystemTimes(&idleTime, &kernelTime, &userTime);
        pre_idleTime = idleTime;
        pre_kernelTime = kernelTime;
        pre_userTime = userTime;
    }

    LONG CompareFileTime(FILETIME time1, FILETIME time2)
    {
        LONG a = time1.dwHighDateTime << 32 | time1.dwLowDateTime ;
        LONG b = time2.dwHighDateTime << 32 | time2.dwLowDateTime ;
        return (b - a);
    }

    int cpuRate(int &cpuOcc, int &cpuIdle, int &cpuUser)
    {
        int res;
        res = GetSystemTimes(&idleTime, &kernelTime, &userTime );

        LONG idle = CompareFileTime(pre_idleTime, idleTime);
        LONG kernel = CompareFileTime(pre_kernelTime, kernelTime);
        LONG user = CompareFileTime(pre_userTime, userTime);

        int cpu_occupancy_rate = (kernel + user - idle) * 100 / (kernel + user);
        //（总的时间 - 空闲时间）/ 总的时间 = 占用CPU时间的比率，即占用率
        int cpu_idle_rate = idle * 100 / (kernel + user);
        //空闲时间 / 总的时间 = 闲置CPU时间的比率，即闲置率
        int cpu_kernel_rate = kernel * 100 / (kernel + user);
        //核心态时间 / 总的时间 = 核心态占用的比率
        int cpu_user_rate = user * 100 / (kernel + user);

        pre_idleTime = idleTime;
        pre_kernelTime = kernelTime;
        pre_userTime = userTime;

        cpuOcc=cpu_occupancy_rate;
        cpuIdle=cpu_idle_rate;
        cpuUser=cpu_user_rate;
        return 0;
    }

    void cpuInfo()
    {
        SYSTEM_INFO  sysInfo;
        OSVERSIONINFOEX osvi;

        GetSystemInfo(&sysInfo);
        qDebug("OemId : %u", sysInfo.dwOemId);
        qDebug("处理器架构 : %u", sysInfo.wProcessorArchitecture);
        qDebug("页面大小 : %u", sysInfo.dwPageSize);
        qDebug("应用程序最小地址 : %u", sysInfo.lpMinimumApplicationAddress);
        qDebug("应用程序最大地址 : %u", sysInfo.lpMaximumApplicationAddress);
        qDebug("处理器掩码 : %u", sysInfo.dwActiveProcessorMask);
        qDebug("处理器数量 : %u", sysInfo.dwNumberOfProcessors);
        qDebug("处理器类型 : %u", sysInfo.dwProcessorType);
        qDebug("虚拟内存分配粒度 : %u", sysInfo.dwAllocationGranularity);
        qDebug("处理器级别 : %u", sysInfo.wProcessorLevel);
        qDebug("处理器版本 : %u", sysInfo.wProcessorRevision);
        osvi.dwOSVersionInfoSize=sizeof(osvi);
        if (GetVersionEx((LPOSVERSIONINFOW)&osvi))
        {
            qDebug("Version     : %u.%u", osvi.dwMajorVersion, osvi.dwMinorVersion);
            qDebug("Build       : %u", osvi.dwBuildNumber);
            qDebug("Service Pack: %u.%u", osvi.wServicePackMajor, osvi.wServicePackMinor);
        }
    }
};

#endif

static SystemInfo systemInfo;

QSystemInfo::QSystemInfo(QObject *parent)
{
    systemInfo.init();
    memset(memoryInfo,0,sizeof memoryInfo);
    cpuUse=0;
}

QSystemInfo::~QSystemInfo()
{

}

int QSystemInfo::getCpuUse() const
{
    return cpuUse;
}

__int64 DiffFileTime(FILETIME time1, FILETIME time2)
{
    __int64 a = time1.dwHighDateTime << 32 | time1.dwLowDateTime;
    __int64 b = time2.dwHighDateTime << 32 | time2.dwLowDateTime;
    return   b - a;
}

void QSystemInfo::run()
{
    int cpuOcc;
    int cpuIdle;
    int cpuUser;
    int cpuuse=0;

    FILETIME idleTime1, idleTime2;
    FILETIME kernelTime1, kernelTime2;
    FILETIME userTime1, userTime2;

    while(!isInterruptionRequested()){
        GetSystemTimes(&idleTime1, &kernelTime1, &userTime1);
        QThread::sleep(2);
        GetSystemTimes(&idleTime2, &kernelTime2, &userTime2);
        int idle = (int)DiffFileTime(idleTime1, idleTime2);
        int kernel = (int)DiffFileTime(kernelTime1, kernelTime2);
        int user = (int)DiffFileTime(userTime1, userTime2);

        if (kernel + user == 0)
            cpuuse = 0.0;
        else
            cpuuse = abs((kernel + user - idle) * 100 / (kernel + user));//（总的时间-空闲时间）/总的时间=占用cpu的时间就是使用率

//        qDebug() << "cpuuse:"<<cpuuse << "%";

        this->cpuUse=cpuuse;

        //内存使用率
        char bufPreCPU[5];
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);
        GlobalMemoryStatusEx(&statex);
        sprintf(bufPreCPU, "%ld%%", statex.dwMemoryLoad); // 内存使用率

//        qDebug() <<bufPreCPU <<"物理内存:"<< statex.ullTotalPhys/1024
//                <<", 可用物理内存:" << statex.ullAvailPhys/1024
//               << ",可用虚拟内存:"<<statex.ullAvailVirtual/1024;

        snprintf(memoryInfo,sizeof memoryInfo, "%d%%,总:%.1lfG,剩:%.2lfG",
                 statex.dwMemoryLoad,
                 (statex.ullTotalPhys*1.0)/1024/1024/1024,
                 (statex.ullAvailPhys*1.0)/1024/1024/1024);
    }
}
