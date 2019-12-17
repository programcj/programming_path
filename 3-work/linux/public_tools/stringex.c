#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "stringex.h"

//十六机制字符串转bin数据
int stringhex2data(const char *hexstring, void *data, int dlen)
{
	const char *p = hexstring;
	unsigned char *v = data;
	int tmp = 0;
	while (*p && dlen)
	{
		if (*p == ' ')
		{
			p++;
			continue;
		}
		sscanf(p, "%02x", &tmp);
		*(v++) = tmp;
		dlen--;
		p += 2;
	}
	return v - (unsigned char *)data;
}

//把bin数据转换成hex字符串
void stringhex_from_data(char **hex_buf, const void *hex_data, int len)
{
	int i = 0;
	char *_buf;
	*hex_buf = (char *)malloc(len * 2 + len + 1);
	if (*hex_buf)
	{
		memset(*hex_buf, 0, len * 2 + len + 1);
		_buf = *hex_buf;
		for (i = 0; i < len; i++)
		{
			sprintf(_buf, "%02X ", 0x00FF & ((char *)hex_data)[i]);
			_buf += 3;
		}
	}
}

//字符串复制,判断src字符串是否为空
char *strcpyex(char *desc, const char *src)
{
	if (src)
		return strcpy(desc, src);
	return NULL;
}

//字符串复制,判断src字符串是否为空
char *strncpyex(char *desc, const char *src, int maxsiz)
{
	char *ptr = desc;
	if (src)
	{
		ptr = strncpy(desc, src, maxsiz);
		//if(maxsiz && maxsiz-1)
		//	dsc[maxsiz-1]=0;
	}
	return ptr;
}

//分割字符串
int strspilts(char *buff, char *spchr, char **spptr, int len)
{
	char *tmp = NULL;
	char *ptr = NULL;
	int spcount = 0;
	ptr = strtok_r(buff, spchr, &tmp);
	if (!ptr)
		return -1;
	while (len && ptr)
	{
		*spptr = ptr;
		spptr++;
		*spptr = tmp;
		spcount++;
		len--;

		if (len)
			ptr = strtok_r(NULL, spchr, &tmp);
	}
	return spcount;
}

/**
 * 寻找子字符串,使用通配符方法
 * substr:
 *   %?:匹配一个字符
 *   %*:匹配n个字符
 *   %%:匹配%字符
 */
char *strstr2(const char *str, const char *substr)
{
	const char *pstr = str;
	const char *psub = substr;
	const char *tmp = NULL, *tmp_next = NULL;
	char b = 0;
	assert(substr != NULL && str != NULL);

	while (*pstr)
	{
		tmp = pstr;

		while (*tmp && *psub)
		{
			if (*psub == '%')
			{
				b = *(psub + 1);
				if (b == '?')
				{			   //log_d("???[%s] [%s]", tmp, psub);
					psub += 2; //bug
					tmp++;
					continue;
				}
				if (b == '*')
				{ //log_d("***[%s] [%s]", tmp, psub+2);
					tmp_next = strstr2(tmp, psub + 2);
					return tmp_next ? (char *)pstr : NULL;
				}
			}

			if (*tmp != *psub)
				break;
			tmp++;
			psub++;
		}
		if (*psub == 0)
			return (char *)pstr;
		psub = substr;
		pstr++;
	}
	return NULL;
}

int str_startwstrcmp_startwithith(const char *str, const char *prefix, int toffset)
{
	str += toffset;
	return strncmp(str, prefix, strlen(prefix));
}

//判断字符串a 是不是以字符串b结尾
int strcmp_endwith(const char *str, const char *suffix)
{
	int len = str ? strlen(str) : 0;
	int len2 = suffix ? strlen(suffix) : 0;
	const char *ptr1 = str + len;
	const char *ptr2 = suffix + len2;
	int ret = 0;

	if (!ptr1 || !ptr2)
		return -1;

	do
	{
		ret = *ptr1 - *ptr2;
		ptr1--;
		ptr2--;
	} while (ptr1 >= str && ptr2 >= suffix && ret == 0);

	return ptr2 >= suffix ? -*ptr2 : ret;
}

char *str_readline(const char *str)
{
	char *lineend;
	char *s;
	size_t len;

	lineend = strchr(str, '\n');
	if (!lineend)
		return NULL;
	lineend++;
	len = lineend - str;
	s = (char *)malloc(len + 1);
	if (!s)
		return s;
	memcpy(s, str, len);
	s[len] = 0;
	return s;
}

