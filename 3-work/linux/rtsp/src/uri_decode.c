
//uri 解码
//rtsp://aff:afa@aff@192.168.0.21:554/dfdf
//rtsp://username:password@192.168.0.12:<port>/xxxxxx
//http://username:password@192.168.0.12:<port>/ | default 80
//https://
//file:///
int uri_decode(const char *url, char _prefix[50], char _user[50],
		char _pass[50], char _host[50], int *_port, char **_path)
{
	const char *prefix_s = url;
	const char *prefix_e = NULL;
	const char *host_s = NULL;
	const char *host_e = NULL;
	const char *user_s = NULL;
	const char *user_e = NULL;
	const char *pass_s = NULL;
	const char *pass_e = NULL;
	const char *path_s = NULL;
	int port = -1;

	{
		const char *pos = strstr(url, "://");
		if (!pos)
			return -1;

		prefix_e = pos;
		//printf("prefix:[%.*s]\n", (int) (prefix_e - prefix_s), prefix_s);
		pos += 3;
		while (*pos && *pos == '/') //去掉:///\/
			pos++;
		host_s = pos;

		pos = strchr(pos, '/');
		if (!pos)
			pos = url + strlen(url);
		path_s = pos; //末尾的目录

		pos = host_s;

		//解析 host信息
		const char *s = pos;
		while (*pos && *pos != '/' && pos < path_s)
		{
			if (*pos == ':')
			{
				//判断后面是否有 user:pass@host   host:123  user:pass@host:123 user:@host:324
				//pos到path_s中是否全是数字
				const char *t = pos + 1;
				while (*t >= '0' && *t <= '9' && t < path_s)
					t++;

				if (t == path_s)
				{
					host_s = s;
					host_e = pos;
					port = atoi(pos + 1);
					break;
				}

				if (!user_s)
				{
					user_s = s;
					user_e = pos;
					s = pos + 1;
					//printf("user:[%.*s]\n", (int) (user_e - user_s), user_s);
				}
				else
				{
					host_s = s;
					host_e = pos;
					port = atoi(pos + 1);
					//printf("host:[%.*s]\n", (int) (host_e - host_s), host_s);
					//printf("port=%d\n", port);
				}
			}

			if (*pos == '@')
			{
				if (!pass_s)
				{
					pass_s = s;
					pass_e = pos;
				}

				//判断后面是否还有'@'
				pass_e = pos;
				s = pos + 1;
				host_s = s;
				//printf("pass:[%.*s]\n", (int) (pass_e - pass_s), pass_s);
			}
			pos++;
		}

		if (!host_e)
			host_e = pos;
		//printf("path=[%s]\n", path_s);
	}

	strncpy(_prefix, prefix_s, prefix_e - prefix_s);
	if (user_s)
		strncpy(_user, user_s, user_e - user_s);
	if (pass_s)
		strncpy(_pass, pass_s, pass_e - pass_s);
	if (host_s)
		strncpy(_host, host_s, host_e - host_s);

	if (port == -1)
	{
		if (strncasecmp(prefix_s, "rtsp", 4) == 0)
			port = 554;
		if (strncasecmp(prefix_s, "https", 5) == 0)
			port = 443;
		else if (strncasecmp(prefix_s, "http", 4) == 0)
			port = 80;
	}
	*_port = port;
	*_path = (char*) path_s;
	return 0;
}