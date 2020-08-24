/*
 * evhttp_ex.h
 *
 *  Created on: 2020年7月16日
 *      Author: cc
 *
 *                .-~~~~~~~~~-._       _.-~~~~~~~~~-.
 *            __.'              ~.   .~              `.__
 *          .'//                  \./                  \\`.
 *        .'//                     |                     \\`.
 *      .'// .-~"""""""~~~~-._     |     _,-~~~~"""""""~-. \\`.
 *    .'//.-"                 `-.  |  .-'                 "-.\\`.
 *  .'//______.============-..   \ | /   ..-============.______\\`.
 *.'______________________________\|/______________________________`.
 *.'_________________________________________________________________`.
 * 
 * 请注意编码格式
 */

#ifndef EVHTTP_EX_H_
#define EVHTTP_EX_H_

#include "cJSON.h"

#include <event2/event.h>
#include <event2/http.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct evbuffer *evbuffer_new_cJSON(cJSON *json);

void evhttp_send_reply_json(struct evhttp_request *req, cJSON *json);

void evhttp_request_resp_origin(struct evhttp_request *req);

//need free
char *evbuffer_to_str(struct evbuffer *buf);

//you need: evhttp_clear_headers(&post_querys);
int evhttp_request_from_param(struct evhttp_request *request, struct evkeyvalq *headers);

void evkeyvalq_debug_printf(struct evkeyvalq *v);

const char *evhttp_cmd_type_tostr(int type);

void evhttp_send_document_cb(struct evhttp_request *req, void *arg);

int evhttp_add_header2(struct evkeyvalq *headers, const char *key, const char *vfmt, ...);

void evhttp_get_cookie(struct evhttp_request* request, const char *name, char *value, int vlen);

#ifdef __cplusplus
}
#endif

#endif /* EVHTTP_EX_H_ */
