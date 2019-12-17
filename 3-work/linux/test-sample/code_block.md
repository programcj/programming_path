# 代码片段收录

### 版本

```
#ifndef CONFIG_SOFTWARE_VERSION 
#define CONFIG_SOFTWARE_VERSION "V0.1"
#endif

CONFIG_SOFTWARE_VERSION=V0.1
```

### 编译时间

```
#ifndef CONFIG_SOFTWARE_BUILD_TMDF 
#define CONFIG_SOFTWARE_BUILD_TMDF __DATE__ " "__TIME__ 
#endif

CONFIG_SOFTWARE_BUILD_TMDF=$(shell date "+%Y%m%d-%H%M%S")
```

## 20191217 显示内存申请与释放情况 

```c
//编译需要添加 -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc -Wl,--wrap=free
static volatile long _memoryUseCount = 0;

void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size)
{
    _memoryUseCount++;
    return __real_malloc(size);
}

void *__real_realloc(void *mem_address, size_t size);
void *__wrap_realloc(void *mem_address, size_t size)
{
    return __real_realloc(mem_address, size);
}

void *__real_calloc(size_t numElements, size_t sizeOfElement);
void *__wrap_calloc(size_t numElements, size_t sizeOfElement)
{
    _memoryUseCount++;
    return __real_calloc(numElements, sizeOfElement);
}

void __real_free(void *ptr);
void __wrap_free(void *ptr)
{
    _memoryUseCount--;
    __real_free(ptr);
}
```
## 片段:显示本程序的线程名
```c
void path_each(const char *path, void (*caldirfun)(const char *ppath, const char *name, void *user),
               void (*calregfun)(const char *ppath, const char *name, void *user), void *user)
{
    DIR *dir = NULL;
    struct dirent *ditem;
    dir = opendir(path);
    if (!dir)
        return;
    while (ditem = readdir(dir))
    {
        if (strcmp(ditem->d_name, ".") == 0 || strcmp(ditem->d_name, "..") == 0)
            continue;
        if (ditem->d_type == DT_DIR)
        {
            if (caldirfun)
                caldirfun(path, ditem->d_name, user);
        }
        if (ditem->d_type == DT_REG)
        {
            if (calregfun)
                calregfun(path, ditem->d_name, user);
        }
    }
    closedir(dir);
}

static void _showTask(const char *ppatch, const char *name, void *user)
{
    char path[1024];
    char taskname[20];
    char *line;

    sprintf(path, "%s%s/comm", ppatch, name);
    FILE *fp = fopen(path, "r");
    memset(taskname, 0, sizeof(taskname));
    if (fp)
    {
        fgets(taskname, sizeof(taskname) - 1, fp);
        fclose(fp);
        if (line = strchr(taskname, '\n'))
        {
            *line = 0;
        }
    }
    printf("\tTask:%s,%s\n", name, taskname);
}

static void *_ThreadMonitor(void *arg)
{
    char path[1024];
    pthread_detach(pthread_self());
    prctl(PR_SET_NAME, "Monitor");
    while (1)
    {
        printf("[%ld]mem use count:%ld\n", time(NULL), _memoryUseCount);
        //6s 显示本机线程
        sprintf(path, "/proc/%d/task/", getpid());
        path_each(path, _showTask, NULL, NULL);
        sleep(6);
    }
    return NULL;
}
```
