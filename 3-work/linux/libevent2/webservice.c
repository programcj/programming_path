/*
 * webserver.c
 *
 *  Created on: 2021年2月26日
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <signal.h>
#include <ctype.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
#else /* !_WIN32 */
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif /* _WIN32 */
#include <signal.h>

#ifdef EVENT__HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef EVENT__HAVE_AFUNIX_H
#include <afunix.h>
#endif

#include <event2/event.h>
#include <event2/http.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include "log.h"

#ifdef _WIN32
#include <event2/thread.h>
#endif /* _WIN32 */

#ifdef EVENT__HAVE_NETINET_IN_H
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#endif

#ifdef _WIN32
#ifndef stat
#define stat _stat
#endif
#ifndef fstat
#define fstat _fstat
#endif
#ifndef open
#define open _open
#endif
#ifndef close
#define close _close
#endif
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif
#endif /* _WIN32 */

#include "libwebservice.h"
#include "evhttp_ex.h"
#include "cJSONEx.h"
#include "libcommon.h"

static int _config_web_port = 8085;

struct httpapi
{
	char *path;
	void (*fun)(struct evhttp_request *req);

	struct httpapi *next;
	struct httpapi *priv;
};

static struct httpapi apis =
			{ "", NULL, &apis, &apis };

void webserver_regedit_api(const char *path, void (*fun)(struct evhttp_request *req))
{
	struct httpapi *api = (struct httpapi*) calloc(sizeof(struct httpapi), 1);
	if (api)
	{
		api->path = strdup(path);
		api->fun = fun;
		api->next = &apis;
		api->priv = apis.priv;
		apis.priv->next = api;
		apis.priv = api;
	}
}

void webservice_init(int web_port)
{
	_config_web_port = web_port;
}

int webservice_getport() 
{
	return _config_web_port;
}

int http_cookie_check(const char *cookieval)
{
	if (strlen(cookieval) == 0)
		return 0;
	return 1;
}

void http_api_send_response(struct evhttp_request *request, int result,
			const char *msg, cJSON *json_data)
{
	cJSON *json = cJSON_CreateObject();
	cJSON_AddStringToObjectEx(json, "result", "%d", result);
	cJSON_AddStringToObject(json, "msg", msg);
	if (json_data)
		cJSON_AddItemToObject(json, "data", json_data);
	evhttp_send_reply_json(request, json);
	cJSON_Delete(json);
}

void http_gencb(struct evhttp_request *request, void *arg)
{
	//const struct evhttp_uri* evhttp_uri = evhttp_request_get_evhttp_uri(request);
	//char url[8192];
	//evhttp_uri_join(const_cast<struct evhttp_uri*>(evhttp_uri), url, 8192);

	const char *uri = evhttp_request_get_uri(request);
	const char *host = evhttp_request_get_host(request);
	int i;
	enum evhttp_cmd_type cmdtype = evhttp_request_get_command(request);

	//	log_d("http", "cmdtype [%s] \n", evhttp_cmd_type_tostr(cmdtype));

	if (EVHTTP_REQ_OPTIONS == cmdtype)
	{
		log_d("http", "options reqeust\n");
		evhttp_add_header(evhttp_request_get_output_headers(request),
					"Access-Control-Allow-Methods", "GET,POST,OPTIONS,PUT");
		evhttp_request_resp_origin(request);

		struct evbuffer *retbuff = evbuffer_new();
		evbuffer_add_printf(retbuff, "success");
		evhttp_send_reply(request, 200, "OK", retbuff);
		evbuffer_free(retbuff);
		return;
	}

#if 0
	if (strncasecmp(uri, "/agapi", 4) == 0 && host)
	{
		struct evhttp_uri *http_uri = evhttp_uri_parse(uri);
		const char *path = evhttp_uri_get_path(http_uri);
		const char *query = evhttp_uri_get_query(http_uri);
		int exists = 0;
		if (!path)
			path = "/";
		if (!query)
			query = "";

		char token[50];
		memset(token, 0, sizeof(token));
		evhttp_get_cookie(request, "LXSESSIONID", token, sizeof(token));
		if (strlen(token) == 0)
		{
			const char *h_token = evhttp_find_header(
				evhttp_request_get_input_headers(request), "token");
			if (h_token)
				strncpy(token, h_token, sizeof(token) - 1);
		}

		log_d("API", "accept request cmdtype:%s, url:%s, LXSESSIONID:[%s]\n",
			evhttp_cmd_type_tostr(cmdtype), uri, token);

		for (i = 0; restful_apis[i].path != NULL; i++)
		{
			if (strcasecmp(path, restful_apis[i].path) != 0)
				continue;
			if (restful_apis[i].auth)
			{
				//需要认证
				if (http_cookie_check(token))
				{
					//方式2:从表单中获取token
					restful_apis[i].cb(request);
				}
				else
				{
					http_api_send_response(request, 401, "Unauthorized", NULL);
				}
			}
			else
			{
				restful_apis[i].cb(request);
			}
			exists = 1;
		}

		if (http_uri)
			evhttp_uri_free(http_uri);

		if (exists)
			return;

		evhttp_request_resp_origin(request);
		evhttp_send_error(request, 404, "404 Document was not found");

		if (cmdtype == EVHTTP_REQ_GET)
		{

		}
		if (cmdtype == EVHTTP_REQ_POST)
		{
			struct evkeyvalq post_querys;
			TAILQ_INIT(&post_querys);

			char *str = evbuffer_to_str(
				evhttp_request_get_input_buffer(request));
			evhttp_parse_query_str(str, &post_querys);
			if (str)
				free(str);
			evkeyvalq_debug_printf(&post_querys);
			evhttp_clear_headers(&post_querys);
		}
		return;
	}
	//1400
	if (strncasecmp(uri, "/VIID/", 6) == 0 && host)
	{
		void http1400api_handle(struct evhttp_request* request);
		http1400api_handle(request);
		return;
	}
#else
	{
		struct httpapi *apifun = NULL;
		int exists = 0;

		struct evhttp_uri *http_uri = evhttp_uri_parse(uri);
		const char *path = evhttp_uri_get_path(http_uri);
		const char *query = evhttp_uri_get_query(http_uri);
		if (!path)
			path = "/";
		if (!query)
			query = "";

		const char* _split = NULL;

		for (apifun = apis.next; apifun != &apis; apifun = apifun->next)
		{
			_split = strchr(apifun->path, '*');
			if(_split)
			{
				int len=_split - apifun->path;
				if (strncasecmp(apifun->path, path, len) == 0 && apifun->fun)
				{
					printf("accept request cmdtype:%s, url:%s\n", evhttp_cmd_type_tostr(cmdtype), uri);
					apifun->fun(request);
					exists = 1;
					break;
				}
			}
			if (strcasecmp(apifun->path, path) == 0 && apifun->fun)
			{
				printf("accept request cmdtype:%s, url:%s\n", evhttp_cmd_type_tostr(cmdtype), uri);
				apifun->fun(request);
				exists = 1;
				break;
			}
		}

		if (http_uri)
			evhttp_uri_free(http_uri);
		if (exists)
			return;
	}
#endif
	evhttp_send_document_cb(request, "htmls");
}

static int
display_listen_sock(struct evhttp_bound_socket *handle)
{
	struct sockaddr_storage ss;
	evutil_socket_t fd;
	ev_socklen_t socklen = sizeof(ss);
	char addrbuf[128];
	void *inaddr;
	const char *addr;
	int got_port = -1;

	fd = evhttp_bound_socket_get_fd(handle);
	memset(&ss, 0, sizeof(ss));
	if (getsockname(fd, (struct sockaddr*) &ss, &socklen))
	{
		perror("getsockname() failed");
		return 1;
	}

	if (ss.ss_family == AF_INET)
	{
		got_port = ntohs(((struct sockaddr_in*) &ss)->sin_port);
		inaddr = &((struct sockaddr_in*) &ss)->sin_addr;
	}
	else if (ss.ss_family == AF_INET6)
	{
		got_port = ntohs(((struct sockaddr_in6*) &ss)->sin6_port);
		inaddr = &((struct sockaddr_in6*) &ss)->sin6_addr;
	}
	else
	{
		fprintf(stderr, "Weird address family %d\n",
					ss.ss_family);
		return 1;
	}

	addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf,
				sizeof(addrbuf));
	if (addr)
	{
		printf("Listening on %s:%d\n", addr, got_port);
		//evutil_snprintf(uri_root, sizeof(uri_root),
		//	"http://%s:%d", addr, got_port);
	}
	else
	{
		fprintf(stderr, "evutil_inet_ntop failed\n");
		return 1;
	}

	return 0;
}

static void api_request_cb(struct evhttp_request *req, void *arg)
{
	struct evhttp_connection *conn;
	enum evhttp_cmd_type method;
	const struct evhttp_uri *uri;
	struct evkeyvalq *headers;

	const char *host, *uri_host, *uri_path, *uri_query, *uri_user_info,
				*uri_scheme;
	ev_uint16_t port;
	char *addr;
	int uri_port;

	//	struct evkeyval *header;
	//	struct evkeyvalq *out_headers;
	struct evbuffer *buffer;

	/* fetch request data */
	conn = evhttp_request_get_connection(req);
	host = evhttp_request_get_host(req);
	evhttp_connection_get_peer(conn, &addr, &port);
	method = evhttp_request_get_command(req);
	uri = evhttp_request_get_evhttp_uri(req);
	headers = evhttp_request_get_input_headers(req);
	uri_host = evhttp_uri_get_host(uri);
	uri_path = evhttp_uri_get_path(uri);
	uri_query = evhttp_uri_get_query(uri);
	uri_user_info = evhttp_uri_get_userinfo(uri);
	uri_scheme = evhttp_uri_get_scheme(uri);
	uri_port = evhttp_uri_get_port(uri);

	/* simple log */
	evkeyvalq_debug_printf(headers);
	//	for (header = headers->tqh_first; header; header = header->next.tqe_next)
	//	{
	//		printf("%s: %s\n", header->key, header->value);
	//	}

	if (host == NULL)
		host = "";

	printf(
				"addr:%s, port:%d, host:%s, method:%d, host:%s, path:%s, query:%s, user info:%s, scheme:%s, port:%d\n",
				addr, port, host, method, uri_host, uri_path, uri_query,
				uri_user_info, uri_scheme, uri_port);

	cJSON *jsonResp = cJSON_CreateObject();
	cJSON_AddStringToObject(jsonResp, "host", host);

	cJSON *jarray = cJSON_CreateArray();
	{
		struct httpapi *apifun = NULL;
		for (apifun = apis.next; apifun != &apis; apifun = apifun->next)
		{
			//if (strcasecmp(apifun->path, uri_path) == 0 && apifun->fun)
			{
				cJSON_AddItemToArray(jarray, cJSON_CreateString(apifun->path));
			}
		}
	}
	cJSON_AddItemToObject(jsonResp, "apis", jarray);

	evhttp_send_reply_json(req, jsonResp);
	cJSON_Delete(jsonResp);
}

void webservice_loop()
{
	struct event_config *cfg = NULL;
	struct event_base *base = NULL;
	struct evhttp *http = NULL;
	struct evhttp_bound_socket *handle = NULL;
	struct evconnlistener *lev = NULL;

	//struct event *term = NULL;
	//struct options o = parse_opts(argc, argv);
	int ret = 0;
#if defined(WIN32) || defined(_WIN32)

#else
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	{
		ret = 1;
		goto err;
	}
#endif
	//setbuf(stdout, NULL);
	//setbuf(stderr, NULL);

	/** Read env like in regress */
	//if (getenv("EVENT_DEBUG_LOGGING_ALL"))
	//	event_enable_debug_logging(EVENT_DBG_ALL);
	cfg = event_config_new();
	base = event_base_new_with_config(cfg);
	if (!base)
	{
		fprintf(stderr, "Couldn't create an event_base: exiting\n");
		ret = 1;
	}
	event_config_free(cfg);
	cfg = NULL;

	http = evhttp_new(base);
	if (!http)
	{
		fprintf(stderr, "couldn't create evhttp. Exiting.\n");
		ret = 1;
	}

	evhttp_set_allowed_methods(http,
				EVHTTP_REQ_GET | EVHTTP_REQ_POST | EVHTTP_REQ_OPTIONS);
	evhttp_set_timeout(http, 60);
	evhttp_set_gencb(http, http_gencb, NULL);
	evhttp_set_cb(http, "/api", api_request_cb, NULL);

	handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", _config_web_port);
	if (!handle)
	{
		fprintf(stderr, "couldn't bind to port %d. Exiting.\n",
					_config_web_port);
		ret = 1;
		goto err;
	}

	if (display_listen_sock(handle))
	{
		ret = 1;
		goto err;
	}

	event_base_dispatch(base);

	err: if (cfg)
		event_config_free(cfg);
	if (http)
		evhttp_free(http);
}

static void* _thread_web(void *arg)
{
	os_thread_setname("WebServer#%d", _config_web_port);
	webservice_loop();
	return NULL;
}

void webservice_start()
{
	log_i("webapi", "WebServer start(port=%d)\r\n", _config_web_port);
	os_thread_run(_thread_web, NULL);
}


