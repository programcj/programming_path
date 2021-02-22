/*
 * evhttp_ex.c
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include "evhttp_ex.h"


char *evbuffer_to_str(struct evbuffer *buf)
{
	int length = 0;
	char *str = NULL;
	if (!buf)
		return strdup("");

	length = evbuffer_get_length(buf);
	if (length > 0)
	{
		str = (char *) malloc(length + 1);
		if (str)
		{
			str[length] = 0;
			evbuffer_remove(buf, str, length);
		}
		else
		{
			str = strdup("");
		}
	}
	return str;
}

struct evbuffer *evbuffer_new_cJSON(cJSON *json)
{
	char *jsonstr = cJSON_Print(json);
	struct evbuffer *retbuff = evbuffer_new();
	evbuffer_add_printf(retbuff, "%s", jsonstr);
	free(jsonstr);
	return retbuff;
}

void evhttp_send_reply_json(struct evhttp_request *req, cJSON *json)
{
	evhttp_request_resp_origin(req);

	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type",
				"application/json; charset=UTF-8");
	struct evbuffer *retbuff = evbuffer_new_cJSON(json);
	evhttp_send_reply(req, 200, "OK", retbuff);
	evbuffer_free(retbuff);
}

//跨域
void evhttp_request_resp_origin(struct evhttp_request *req)
{
	evhttp_add_header(evhttp_request_get_output_headers(req),
				"Access-Control-Allow-Origin", "*");
	//	evhttp_add_header(evhttp_request_get_output_headers(req),
	//				"Access-Control-Allow-Methods", "GET,POST, PUT");
	evhttp_add_header(evhttp_request_get_output_headers(req),
				"Access-Control-Allow-Headers",
				"Origin, X-Requested-With, Content-Type, Accept, token");
}

void evkeyvalq_debug_printf(struct evkeyvalq *v)
{
	struct evkeyval *item;

	for (item = v->tqh_first; item; item = item->next.tqe_next)
	{
		printf("[%s]:[%s]\n", item->key, item->value);
	}
}

//you need: evhttp_clear_headers(&post_querys);
int evhttp_request_from_param(struct evhttp_request *request, struct evkeyvalq *headers)
{
	struct evkeyvalq querys;
	const char *uri = evhttp_request_get_uri(request);

	if (EVHTTP_REQ_GET == evhttp_request_get_command(request))
	{
		uri = uri ? uri : "";
		evhttp_parse_query(uri, headers); //内部会 TAILQ_INIT(headers);
		return 0;
	}
	else if (EVHTTP_REQ_POST == evhttp_request_get_command(request))
	{
		char *str = evbuffer_to_str(evhttp_request_get_input_buffer(request));
		evhttp_parse_query_str(str, headers);
		if (str)
			free(str);
		return 0;
	}
	else
	{
		TAILQ_INIT(headers);
	}
	return -1;
}

const char *evhttp_cmd_type_tostr(int type)
{
	switch (type)
	{
		case EVHTTP_REQ_GET:
			return "GET";
		case EVHTTP_REQ_POST:
			return "POST";
		case EVHTTP_REQ_HEAD:
			return "HEAD";
		case EVHTTP_REQ_PUT:
			return "PUT";
		case EVHTTP_REQ_DELETE:
			return "DELETE";
		case EVHTTP_REQ_OPTIONS:
			return "OPTIONS";
		case EVHTTP_REQ_TRACE:
			return "TRACE";
		case EVHTTP_REQ_CONNECT:
			return "CONNECT";
		case EVHTTP_REQ_PATCH:
			return "PATCH";
		default:
			return "NONE";
	}
}

static const struct table_entry
{
	const char *extension;
	const char *content_type;
} content_type_table[] =
			{
						{ "txt", "text/plain" },
						{ "c", "text/plain" },
						{ "h", "text/plain" },
						{ "html", "text/html" },
						{ "htm", "text/htm" },
						{ "css", "text/css" },
						{ "gif", "image/gif" },
						{ "jpg", "image/jpeg" },
						{ "jpeg", "image/jpeg" },
						{ "png", "image/png" },
						{ "pdf", "application/pdf" },
						{ "ps", "application/postscript" },
						{ NULL, NULL },
			};

static const char *guess_content_type(const char *path)
{
	const char *last_period, *extension;
	const struct table_entry *ent;
	last_period = strrchr(path, '.');
	if (!last_period || strchr(last_period, '/'))
		goto not_found;
	/* no exension */
	extension = last_period + 1;
	for (ent = &content_type_table[0]; ent->extension; ++ent)
	{
		if (!evutil_ascii_strcasecmp(ent->extension, extension))
			return ent->content_type;
	}

	not_found:
	return "application/misc";
}

void evhttp_send_document_cb(struct evhttp_request *req, void *arg)
{
	struct evbuffer *evb = NULL;
	const char *docroot = arg;
	const char *uri = evhttp_request_get_uri(req);
	struct evhttp_uri *decoded = NULL;
	const char *path;
	char *decoded_path;
	char *whole_path = NULL;
	size_t len;
	int fd = -1;
	struct stat st;

	if (evhttp_request_get_command(req) != EVHTTP_REQ_GET)
	{
		//dump_request_cb(req, arg);
		evhttp_request_resp_origin(req);
		evhttp_send_error(req, 404, "Document was not found");
		return;
	}

	printf("Got a GET request for <%s>\n", uri);

	/* Decode the URI */
	decoded = evhttp_uri_parse(uri);
	if (!decoded)
	{
		printf("It's not a good URI. Sending BADREQUEST\n");
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}

	/* Let's see what path the user asked for. */
	path = evhttp_uri_get_path(decoded);
	if (!path)
		path = "/";
	if (strcmp(path, "/") == 0)
		path = "/index.html";

	/* We need to decode it, to see what path the user really wanted. */
	decoded_path = evhttp_uridecode(path, 0, NULL);
	if (decoded_path == NULL)
		goto err;
	/* Don't allow any ".."s in the path, to avoid exposing stuff outside
	 * of the docroot.  This test is both overzealous and underzealous:
	 * it forbids aceptable paths like "/this/one..here", but it doesn't
	 * do anything to prevent symlink following." */
	if (strstr(decoded_path, ".."))
		goto err;

	len = strlen(decoded_path) + strlen(docroot) + 2;
	if (!(whole_path = malloc(len)))
	{
		perror("malloc");
		goto err;
	}
	evutil_snprintf(whole_path, len, "%s/%s", docroot, decoded_path);

	if (stat(whole_path, &st) < 0)
	{
		goto err;
	}

	/* This holds the content we're sending. */
	evb = evbuffer_new();

	if (S_ISDIR(st.st_mode))
	{
#if 0
		/* If it's a directory, read the comments and make a little
		 * index page */
		DIR *d;
		struct dirent *ent;
		const char *trailing_slash = "";

		if (!strlen(path) || path[strlen(path) - 1] != '/')
		trailing_slash = "/";

		if (!(d = opendir(whole_path)))
		goto err;

		evbuffer_add_printf(evb,
					"<!DOCTYPE html>\n"
					"<html>\n <head>\n"
					"  <meta charset='utf-8'>\n"
					"  <title>%s</title>\n"
					"  <base href='%s%s'>\n"
					" </head>\n"
					" <body>\n"
					"  <h1>%s</h1>\n"
					"  <ul>\n",
					decoded_path, /* XXX html-escape this. */
					path, /* XXX html-escape this? */
					trailing_slash,
					decoded_path /* XXX html-escape this */);

		while ((ent = readdir(d)))
		{
			const char *name = ent->d_name;
			evbuffer_add_printf(evb,
						"    <li><a href=\"%s\">%s</a>\n",
						name, name);/* XXX escape this */
		}
		evbuffer_add_printf(evb, "</ul></body></html>\n");
		closedir(d);
		evhttp_add_header(evhttp_request_get_output_headers(req),
					"Content-Type", "text/html");
#else
		evhttp_send_error(req, 404, "Document was not found");
#endif
	}
	else
	{
		/* Otherwise it's a file; add it to the buffer to get
		 * sent via sendfile */
		const char *type = guess_content_type(decoded_path);
		if ((fd = open(whole_path, O_RDONLY)) < 0)
		{
			perror("open");
			goto err;
		}

		if (fstat(fd, &st) < 0)
		{
			/* Make sure the length still matches, now that we
			 * opened the file :/ */
			perror("fstat");
			goto err;
		}
		evhttp_add_header(evhttp_request_get_output_headers(req),
					"Content-Type", type);
		evbuffer_add_file(evb, fd, 0, st.st_size);
	}

	evhttp_send_reply(req, 200, "OK", evb);
	goto done;

	err:
	evhttp_request_resp_origin(req);
	evhttp_send_error(req, 404, "Document was not found");
	if (fd >= 0)
		close(fd);
	done:
	if (decoded)
		evhttp_uri_free(decoded);
	if (decoded_path)
		free(decoded_path);
	if (whole_path)
		free(whole_path);
	if (evb)
		evbuffer_free(evb);
}

int evhttp_add_header2(struct evkeyvalq *headers, const char *key, const char *vfmt, ...)
{
	va_list v;
	char buff[300];
	va_start(v, vfmt);
	vsnprintf(buff, sizeof(buff) - 1, vfmt, v);
	va_end(v);
	return evhttp_add_header(headers, key, buff);
}

void evhttp_get_cookie(struct evhttp_request* request, const char *name, char *value, int vlen)
{
	const char *Cookie = evhttp_find_header(evhttp_request_get_input_headers(request), "Cookie");
	Cookie = Cookie ? Cookie : "";
	const char *ptr = strstr(Cookie, name); //"LXSESSIONID=");
	if (ptr)
	{
		ptr += strlen(name);
		if (*ptr == '=')
			ptr++;
		while (vlen > 0 && *ptr && *ptr != ';')
		{
			*value++ = *ptr++;
		}
		*value = 0;
	}
}

