/*
 * evhttp_ex.c
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdarg.h>

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

#include "evhttp_ex.h"

char* evbuffer_to_str(struct evbuffer* buf)
{
	int length = 0;
	char* str = NULL;
	if (!buf)
		return strdup("");

	length = evbuffer_get_length(buf);
	if (length > 0)
	{
		str = (char*)malloc(length + 1);
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

struct evbuffer* evbuffer_new_cJSON(cJSON* json)
{
	char* jsonstr = cJSON_Print(json);
	struct evbuffer* retbuff = evbuffer_new();
	evbuffer_add_printf(retbuff, "%s", jsonstr);
	free(jsonstr);
	return retbuff;
}

void evhttp_send_reply_json(struct evhttp_request* req, cJSON* json)
{
	evhttp_request_resp_origin(req);

	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type",
		"application/json; charset=UTF-8");
	struct evbuffer* retbuff = evbuffer_new_cJSON(json);
	evhttp_send_reply(req, 200, "OK", retbuff);
	evbuffer_free(retbuff);
}

//跨域
void evhttp_request_resp_origin(struct evhttp_request* req)
{
	evhttp_add_header(evhttp_request_get_output_headers(req),
		"Access-Control-Allow-Origin", "*");
	//	evhttp_add_header(evhttp_request_get_output_headers(req),
	//				"Access-Control-Allow-Methods", "GET,POST, PUT");
	evhttp_add_header(evhttp_request_get_output_headers(req),
		"Access-Control-Allow-Headers",
		"Origin, X-Requested-With, Content-Type, Accept, token, Referer");
}

void evkeyvalq_debug_printf(struct evkeyvalq* v)
{
	struct evkeyval* item;

	for (item = v->tqh_first; item; item = item->next.tqe_next)
	{
		printf("[%s]:[%s]\n", item->key, item->value);
	}
}

//you need: evhttp_clear_headers(&post_querys);
int evhttp_request_from_param(struct evhttp_request* request, struct evkeyvalq* headers)
{
	const char* uri = evhttp_request_get_uri(request);

	if (EVHTTP_REQ_GET == evhttp_request_get_command(request))
	{
		uri = uri ? uri : "";
		evhttp_parse_query(uri, headers); //内部会 TAILQ_INIT(headers);
		return 0;
	}
	else if (EVHTTP_REQ_POST == evhttp_request_get_command(request))
	{
		char* str = evbuffer_to_str(evhttp_request_get_input_buffer(request));
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

const char* evhttp_cmd_type_tostr(int type)
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
	const char* extension;
	const char* content_type;
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
			{ "js", "text/javascript; charset=utf-8" },
			{ "mp4", "video/mp4" },
			{ NULL, NULL },
};

static const char* guess_content_type(const char* path)
{
	const char* last_period, * extension;
	const struct table_entry* ent;
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

void evhttp_send_reply_file(struct evhttp_request* req, const char* filepath)
{
	int flen = 0;
	struct stat st;

	memset(&st, 0, sizeof(st));
	if (stat(filepath, &st) < 0)
	{
	}
	flen = st.st_size;

	evhttp_request_resp_origin(req);

	struct evbuffer* retbuff = evbuffer_new();

	if (flen > 0)
	{
		char* buff = (char*)malloc(flen);
		if (buff != NULL)
		{
			FILE* fp = fopen(filepath, "rb");
			if (fp)
			{
				fread(buff, flen, 1, fp);
				fclose(fp);

				evbuffer_add(retbuff, buff, flen);
			}
			free(buff);
		}
	}

	evhttp_send_reply(req, 200, "OK", retbuff);
	evbuffer_free(retbuff);
	//evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type",
	//	"application/json; charset=UTF-8");

}

#ifdef _WIN32
#else
int _sort(const struct dirent** __e1,
	const struct dirent** __e2)
{
	//struct stat info1, info2;
	//   string file1_name((*__e1)->d_name);
	//   string file2_name((*__e2)->d_name);
	//   stat(("/sdcard/mmcblk0p1/" + file1_name).c_str(), &info1);
	//   stat(("/sdcard/mmcblk0p1/" + file2_name).c_str(), &info2);
	return strcmp((*__e2)->d_name, (*__e1)->d_name);
}
#endif

void evhttp_send_document_cb(struct evhttp_request* req, const char* proxypath, const void* arg, int flaglistfiles)
{
	struct evbuffer* evb = NULL;
	const char* docroot = arg;
	const char* uri = evhttp_request_get_uri(req);
	struct evhttp_uri* decoded = NULL;
	const char* path;
	char* decoded_path;
	char* whole_path = NULL;
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

	//printf("Got a GET request for <%s>\n", uri);

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

	if (strlen(proxypath) > 0)
	{
		if (0 == strncmp(decoded_path, proxypath, sizeof(proxypath)))
		{
			//path += strlen(proxypath);
			evutil_snprintf(whole_path, len, "%s/%s", docroot, decoded_path + strlen(proxypath));
		}
	}
	else
	{
		evutil_snprintf(whole_path, len, "%s/%s", docroot, decoded_path);
	}

	if (stat(whole_path, &st) < 0)
	{
		goto err;
	}

	/* This holds the content we're sending. */
	evb = evbuffer_new();

	if (S_ISDIR(st.st_mode))
	{
		if (flaglistfiles)
		{
			const char* trailing_slash = "";
			if (!strlen(path) || path[strlen(path) - 1] != '/')
				trailing_slash = "/";

			char filepath[1024];

#ifdef _WIN32
			HANDLE d;
			WIN32_FIND_DATAA ent;
			char* pattern;
			size_t dirlen;
			dirlen = strlen(whole_path);
			pattern = malloc(dirlen + 3);
			memcpy(pattern, whole_path, dirlen);
			pattern[dirlen] = '\\';
			pattern[dirlen + 1] = '*';
			pattern[dirlen + 2] = '\0';
			d = FindFirstFileA(pattern, &ent);
			free(pattern);
			if (d == INVALID_HANDLE_VALUE)
				goto err;
			evbuffer_add_printf(evb,
				//"<!DOCTYPE html>\n"
				"<html>\n <head>\n"
				//"  <meta charset='utf-8'>\n"
				"  <title>%s</title>\n"
				"  <base href='%s%s'>\n"
				" </head>\n"
				" <body>\n"
				"  <h1>Index of %s</h1><hr><pre>",
				decoded_path, /* XXX html-escape this. */
				path, /* XXX html-escape this? */
				trailing_slash,
				decoded_path /* XXX html-escape this */);
			do {
				const char* name = ent.cFileName;
				snprintf(filepath, sizeof(filepath), "%s/%s", whole_path, name);
				stat(filepath, &st); //需要判断是否是目录
				struct tm* tm = localtime(&st.st_mtime);

				if (FILE_ATTRIBUTE_DIRECTORY & ent.dwFileAttributes)
				{
					evbuffer_add_printf(evb, "<a href=\"%s\">%s</a>\n", name, name);/* XXX escape this */
				}
				else {
					evbuffer_add_printf(evb, "<a href=\"%s\">%s</a> \t %d/%d/%d %02d:%02d:%02d \t %ldB\n",
						name, name,
						tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
						tm->tm_hour, tm->tm_min, tm->tm_sec,
						st.st_size
					);
				}
				
			} while (FindNextFileA(d, &ent));
			FindClose(d);
#else
			struct dirent** namelist;
			int n;

			//过滤，排序
			n = scandir(whole_path, &namelist, NULL, _sort);
			if (n < 0)
			{
				perror("scandir");
				goto err;
			}
			evbuffer_add_printf(evb,
				//"<!DOCTYPE html>\n"
				"<html>\n <head>\n"
				//"  <meta charset='utf-8'>\n"
				"  <title>%s</title>\n"
				"  <base href='%s%s'>\n"
				" </head>\n"
				" <body>\n"
				"  <h1>Index of %s</h1><hr><pre>",
				decoded_path, /* XXX html-escape this. */
				path, /* XXX html-escape this? */
				trailing_slash,
				decoded_path /* XXX html-escape this */);
			{
				struct stat st;
				
				char tmstr[20];
				while (n--)
				{
					//printf("%s\n", namelist[n]->d_name);
					snprintf(filepath, sizeof(filepath), "%s/%s", whole_path, namelist[n]->d_name);
					memset(&st, 0, sizeof(st));
					stat(filepath, &st); //需要判断是否是目录
					struct tm* tm = localtime(&st.st_mtime);

					if (namelist[n]->d_type == DT_DIR)
					{
						evbuffer_add_printf(evb, "<a href=\"%s\">%s</a> \n",
							namelist[n]->d_name,
							namelist[n]->d_name
						);/* XXX escape this */
					}
					else
					{
						evbuffer_add_printf(evb, "<a href=\"%s\">%s</a> \t %d/%d/%d %02d:%02d:%02d \t %ldByte\n",
							namelist[n]->d_name,
							namelist[n]->d_name,
							tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
							tm->tm_hour, tm->tm_min, tm->tm_sec,
							st.st_size
						);/* XXX escape this */
					}

					free(namelist[n]);  //------------------------>①每访问完一条目录或文件信息，释放由malloc分配的用于存放该信息的动态内存
				}
				free(namelist);         //------------------------>②访问完指定目录下所有目录或文件信息内容，释放malloc分配的用于索引的指针数组内存
			}
			//DIR *d;
			//struct dirent *ent;
			//if (!(d = opendir(whole_path)))
			//	goto err;
//				evbuffer_add_printf(evb,
//																					"<!DOCTYPE html>\n"
//																								"<html>\n <head>\n"
//																								"  <meta charset='utf-8'>\n"
//																								"  <title>%s</title>\n"
//																								"  <base href='%s%s'>\n"
//																								" </head>\n"
//																								" <body>\n"
//																								"  <h1>%s</h1>\n"
//																								"  <ul>\n",
//																					decoded_path, /* XXX html-escape this. */
//																					path, /* XXX html-escape this? */
//																					trailing_slash,
//																					decoded_path /* XXX html-escape this */);
			//while ((ent = readdir(d)))
			//{
			//		const char *name = ent->d_name;
			//		evbuffer_add_printf(evb,"<li><a href=\"%s\">%s</a>\n", name, name);/* XXX escape this */
			//}
			//closedir(d);
#endif
			evbuffer_add_printf(evb, "</pre><hr></body></html>\n");
		}
		else
		{
			evhttp_send_error(req, 404, "Document was not found");
		}
	}
	else
	{
		/* Otherwise it's a file; add it to the buffer to get
		 * sent via sendfile */
		const char* type = guess_content_type(decoded_path);

		if (stat(whole_path, &st) < 0)
			//if (fstat(fd, &st) < 0)
		{
			/* Make sure the length still matches, now that we
			 * opened the file :/ */
			perror("fstat");
			goto err;
		}

		//if ((fd = open(whole_path, O_RDONLY)) < 0)
		//{
		//	perror("open");
		//	goto err;
		//}
		evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", type);
		int flen = st.st_size;

		char* buff = (char*)malloc(flen);
		if (buff == NULL)
		{
			goto err;
		}

		FILE* fp = fopen(whole_path, "rb");
		if (fp)
		{
			fread(buff, flen, 1, fp);
			fclose(fp);
		}
		evbuffer_add(evb, buff, flen);
		free(buff);
		//evbuffer_add_file(evb, fd, 0, st.st_size); //通过这个方式出现文件锁住问题
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

int evhttp_add_header2(struct evkeyvalq* headers, const char* key, const char* vfmt, ...)
{
	va_list v;
	char buff[300];
	va_start(v, vfmt);
	vsnprintf(buff, sizeof(buff) - 1, vfmt, v);
	va_end(v);
	return evhttp_add_header(headers, key, buff);
}

void evhttp_get_cookie(struct evhttp_request* request, const char* name, char* value, int vlen)
{
	const char* Cookie = evhttp_find_header(evhttp_request_get_input_headers(request), "Cookie");
	Cookie = Cookie ? Cookie : "";
	const char* ptr = strstr(Cookie, name); //"LXSESSIONID=");
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

void evhttp_send_reply_json_text(struct evhttp_request* req, cJSON* json)
{
	evhttp_request_resp_origin(req);
	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html; charset=UTF-8");

	struct evbuffer* retbuff = evbuffer_new_cJSON(json);
	evhttp_send_reply(req, 200, "OK", retbuff);
	evbuffer_free(retbuff);
}

const char* evhttp_find_header_string(struct evkeyvalq* h, const char* name, const char* defv)
{
	const char* v = evhttp_find_header(h, name);
	if (v == NULL)
		return defv;
	return v;
}

int evhttp_find_header_number(struct evkeyvalq* h, const char* name, int defv)
{
	const char* v = evhttp_find_header(h, name);
	if (v == NULL)
		return defv;
	return atoi(v);
}
