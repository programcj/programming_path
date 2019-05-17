#include "BindServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>
#include <sys/select.h>
#include <unistd.h>

#include "clog.h"
#include "list.h"

unsigned long clock_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

struct ZTimer {
    struct list_head list;
    void *context;
    void (*callback)(void *);
    unsigned long interval;
};

enum zsheme_t {
    zsheme_AUTO,
    zsheme_NOT,
    zsheme_S0,
    zsheme_S2,
};


struct ZSerial {
    int fd;
//    struct list_head zbuflist_recv;
//    struct list_head zbuflist_write;
    
    struct list_head list_timer;
    
    //application
    struct list_head _list_zwsenddata;
    struct list_head _list_appsendata;
    struct list_head _list_sendreq;
};

struct ZSerial zserial;

void ZSerial_init(){
    INIT_LIST_HEAD(&zserial._list_appsendata);
    INIT_LIST_HEAD(&zserial._list_sendreq);
    INIT_LIST_HEAD(&zserial._list_zwsenddata);
    INIT_LIST_HEAD(&zserial.list_timer);
}

void ztimer_handle(struct ZSerial *t){
    struct list_head *p=NULL,*n=NULL;
    struct ZTimer *titem=0;
    unsigned long now=clock_time();
    
    list_for_each_safe(p, n, &t->list_timer){
        titem=list_entry(p, struct ZTimer, list);
        if(now >= titem->interval){
            log_d("ztimer (%ld - %ld)", now, titem->interval);
            list_del(p);
            if(titem->callback)
                titem->callback(titem->context);
        }
    }
}



void ztime_add(struct ZTimer *t, void *context){
    
}

void ztime_set(struct ZTimer *t, int timer, void (*callback)(void *), void *context){
    t->callback=callback;
    t->context=context;
    t->interval=clock_time()+timer;
    list_add(&t->list, &zserial.list_timer);
}

//===========================================

typedef void (*zw_sendrequest_callback_t)(int status, void *context);

typedef void (*zw_appsenddata_callback_t)(int status, void *context);

struct zw_sendreq_t {
    struct list_head list;
    unsigned int timeout;
    struct ZTimer timer;
    uint8_t node;
    zw_sendrequest_callback_t callback;
    void *context;
};

void list_sendrequest_add(struct zw_sendreq_t *t){
    list_add(&t->list, &zserial._list_sendreq);
}

void zw_request_timeout(void *context){
    struct zw_sendreq_t *sendreq=(struct zw_sendreq_t*)context;
    struct list_head *p,*n;
    struct zw_sendreq_t *item=NULL;
    
    list_for_each_safe(p, n, &zserial._list_sendreq){
        item=list_entry(p, struct zw_sendreq_t, list);
        if(item==sendreq){
            list_del(p);
            break;
        }
    }
    if(sendreq->callback){
        sendreq->callback(-1, sendreq->context);
    }
    free(sendreq);
    log_d("request response timeout");
}

void zw_appsenddata_callback(int status, void *context) {
    struct zw_sendreq_t *sendreq=(struct zw_sendreq_t*)context;
    log_d("request ok ->%d, wait response , timeout is %ld ", status, sendreq->timeout);
    ztime_set(&sendreq->timer,sendreq->timeout,zw_request_timeout, sendreq);
}

extern int zw_appsenddata(uint8_t node, uint8_t *data, uint8_t dlen,zw_appsenddata_callback_t callback, void *context);

int zw_sendrequest(uint8_t node, uint8_t *data, uint8_t dlen,uint8_t waitcls, uint8_t waitcmd,int timeout, zw_sendrequest_callback_t callback, void *context){
    
    struct zw_sendreq_t *sendreq=(struct zw_sendreq_t *)malloc(sizeof(struct zw_sendreq_t));
    sendreq->timeout=timeout;
    sendreq->callback=callback;
    sendreq->context=context;
    list_sendrequest_add(sendreq);
    zw_appsenddata(node, data, dlen, zw_appsenddata_callback,sendreq);
    return 0;
}

//=============================


struct zw_appsenddata_t {
    struct list_head list;
    zw_appsenddata_callback_t callback;
    void *context;
    uint8_t node;
    uint8_t data[0xFF];
    int datalen;
    enum zsheme_t zsheme;
};

int zw_appsenddata(uint8_t node, uint8_t *data, uint8_t dlen,zw_appsenddata_callback_t callback, void *context){
    struct zw_appsenddata_t *item=(struct zw_appsenddata_t*)malloc(sizeof(struct zw_appsenddata_t));
    item->callback=callback;
    item->context=context;
    memcpy(item->data,data,dlen);
    item->datalen=dlen;
    item->zsheme=zsheme_NOT;
    list_add(&item->list, &zserial._list_appsendata);
    return 0;
}

int _lock_appsenddata=0;

void zw_senddata_callback(int status, void *context){
    struct zw_appsenddata_t *item=(struct zw_appsenddata_t *)context;
    if(item->callback){
        item->callback(status, item->context);
    }
    free(item);
}

int zw_senddata(uint8_t node, uint8_t *data, uint8_t datalen, zw_appsenddata_callback_t callback, void *context);

int zw_appsenddata_loop(){
    struct list_head *p,*n;
    struct zw_appsenddata_t *item=NULL;
    
    list_for_each_safe(p, n, &zserial._list_appsendata){
        item=list_entry(p, struct zw_appsenddata_t, list);
        list_del(p);
    }
    if(item==NULL)
        return 0;
    switch(item->zsheme){
        case zsheme_AUTO:
            break;
        case zsheme_NOT:
            zw_senddata(item->node,item->data,item->datalen,zw_senddata_callback,item);
            break;
        case zsheme_S0:
            
            break;
        case zsheme_S2:
            break;
    }
    return 0;
}

int zw_senddata(uint8_t node, uint8_t *data, uint8_t datalen, zw_appsenddata_callback_t callback, void *context){
    struct zw_appsenddata_t *item=(struct zw_appsenddata_t*)malloc(sizeof(struct zw_appsenddata_t));
    item->callback=callback;
    item->context=context;
    memcpy(item->data,data, datalen);
    item->datalen=datalen;
    item->node=node;
    list_add(&item->list, &zserial._list_zwsenddata);
    return 0;
}

int _lock_zwsendata=0;

void zw_send_data_callback_func(int status,int txtype){
    struct list_head *p,*n;
    struct zw_appsenddata_t *item=NULL;
    
    list_for_each_safe(p, n, &zserial._list_zwsenddata) {
        item=list_entry(p, struct zw_appsenddata_t, list);
        list_del(p);
        break;
    }
    if(item) {
        if(item->callback){
            item->callback(status, item->context);
        }
        free(item);
    }
    _lock_zwsendata=0;
}

int ZW_SendData(uint8_t node, uint8_t *data, uint8_t datalen){
    log_hex(data, datalen, "senddata->(%d)", node);
    return 0;
}


void zw_sendrequest_callback(int status, void *context){
    log_d("send request status:%d", status);
}

void zserial_loop(struct ZSerial *t){
    struct list_head *p,*n;
    struct zw_appsenddata_t *item=NULL;
    int _inputfun;
    fd_set fds;
    struct timeval tv;
    int delay;
    
    while(1){
        zw_appsenddata_loop();
        if(!_lock_zwsendata){
            list_for_each_safe(p, n, &t->_list_zwsenddata) {
                item=list_entry(p, struct zw_appsenddata_t, list);
                break;
            }
            if(item){
                ZW_SendData(item->node,item->data, item->datalen);
                _lock_zwsendata=1;
                item=NULL;
                zw_send_data_callback_func(0,0);
            }
        }
        
        ztimer_handle(t);
        
       
        
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        
        delay = 200;
        tv.tv_sec = delay / 1000;
        tv.tv_usec = (delay % 1000)*1000;
        
        if(select(STDIN_FILENO+1, &fds, NULL, NULL, &tv)>0){
            log_d("Please input:");
            _inputfun=0;
            read(STDIN_FILENO, &_inputfun, 1);
            log_d("Please input ok");
            switch (_inputfun) {
                case 10:
                    
                    break;
                case 'a':{
                    uint8_t data[100];
                    int dlen=100;
                    memset(data,'A', sizeof(data));
                    
                    zw_sendrequest(3, data, dlen, 0x20, 0x03, 3*1000, zw_sendrequest_callback, NULL);
                    
                }
                    break;
                default:
                    break;
            }

        }
        
    }
}

int BindServer_loop(BindServer handle){
    ZSerial_init();
    log_d("run...");
    zserial_loop(&zserial);
    return NULL;
}
