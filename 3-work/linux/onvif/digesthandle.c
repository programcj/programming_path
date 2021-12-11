#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "digest.h"
#include "client.h"

#define HTTP_DIGEST_END_HEADER "<!DOCTYPE html>"
#define HTTP_DIGEST_START_HEADER "WWW-Authenticate: "

// 生成32位随机数字
// char *CreateRandomNum(int N)
// {
//      int flag;
// 	 int k=0,j=0;
// 	 char *random_str =  (char*)malloc(N+1);
// 	 random_str[0] = '\0';
// 	 // 1970到现在的时间sec作为种子
// 	 unsigned int seed = (unsigned)time(NULL);
// 	 srand(seed);
// 	 for(j=0;j<N;j++)
// 	  {
// 		   unsigned int random_num = rand();
// 		   flag = random_num%3;
// 		   if(flag == 0)
// 		   {
// 			   random_str[k++]='0'+random_num%10;
// 		   }
// 		   else if(flag == 1)
// 		   {
// 			   random_str[k++]='a'+random_num%26;
// 		   }
// 		   else if(flag == 2)
// 		   {
// 			   random_str[k++]='A'+random_num%26;
// 		   }
// 		   srand(random_num);
// 	  }
// 	 random_str[k]='\0';
// 	 return random_str;
// }

static void _CreateRandom32(char *randstr) 
{
	int i = 0;
	char* ptr = randstr;
	unsigned short v = 0;
	unsigned int seed = (unsigned) time(NULL);
	srand(seed); /*播种子*/
	//cnonce="F9FE061E707F4091EEBEA6E9E0A2A7B2",
	//cnonce="400616322553302B623F0A0C514B0543",
	for (i = 0; i < 32; i += 2) {
		v = rand() % 100;/*产生随机整数*/
		sprintf(ptr, "%02X", v);
		ptr += 2;
	}
}

// 创建Http Digest认真字符串
// model=0 自动生成随机cnonce, model=1 使用传入的指定指定cnonce
int HttpDigestCreater(char *DisgetStr, char *DisgetResult,
		int DisgetResultLimit, char *IP, char *username, char *password,
		char *cnonce, int model) {
	//char result[4096] = {0};

	// 定义认证内容
	digest_t d;
	char randcnonce[32] = { 0 };
	const char *p_opaque = NULL;

	p_opaque = strstr(DisgetStr, "opaque=");

	if (-1 == digest_is_digest(DisgetStr)) {
		//PRINTF_ERR("Is not a digest string! \n");
		return -1;
	}

	if (-1 == digest_init(&d)) {
		//PRINTF_ERR("Could not init digest context! \n");
		return -1;
	}

	if (0 == digest_client_parse(&d, DisgetStr)) {
		//PRINTF_ERR("Could not parse digest string! \n");
		return -1;
	}

	if (0 == model) {
		//需要生成随机字符串
		//randcnonce = CreateRandomNum(32);
		_CreateRandom32(randcnonce);
	}

	digest_set_attr(&d, D_ATTR_USERNAME, (digest_attr_value_tptr) username);
	digest_set_attr(&d, D_ATTR_PASSWORD, (digest_attr_value_tptr) password);
	digest_set_attr(&d, D_ATTR_URI, (digest_attr_value_tptr) IP);

	if (0 == model) {
		digest_set_attr(&d, D_ATTR_CNONCE, (digest_attr_value_tptr) randcnonce);
	} else if (1 == model) {
		digest_set_attr(&d, D_ATTR_CNONCE, (digest_attr_value_tptr) cnonce);
	}

	digest_set_attr(&d, D_ATTR_ALGORITHM, (digest_attr_value_tptr) 1);

	if (!p_opaque)
		digest_set_attr(&d, D_ATTR_OPAQUE, (digest_attr_value_tptr) "");

	digest_set_attr(&d, D_ATTR_METHOD,(digest_attr_value_tptr) DIGEST_METHOD_POST);

#if 0
	if (-1 == digest_client_generate_header(&d, result, sizeof (result))) {
		PRINTF_ERR ("Could not build the Authorization header! \n");
		return -1;
	}

	sprintf(DisgetResult, "Authorization: %s", result);
#else
	sprintf(DisgetResult, "Authorization: ");

	int DisgetResultHeadLen = strlen(DisgetResult);
	if (-1
			== digest_client_generate_header(&d,
					DisgetResult + DisgetResultHeadLen,
					DisgetResultLimit - DisgetResultHeadLen - 1)) {
		//PRINTF_ERR("Could not build the Authorization header! \n");
		return -1;
	}
#endif

	return 0;
}

// 解析Onvif鉴权信息
int ParserOnvifHttpDigest(char *OnvifHttpDigestStr, char *ParserHandleStr,
		int ParserHandleStrLimit) {
	char *src = NULL;
	char *p1 = NULL;
	char *p2 = NULL;
	int len = 0;

	src = OnvifHttpDigestStr;
	p1 = strstr(src, HTTP_DIGEST_START_HEADER);
	p2 = strstr(src, HTTP_DIGEST_END_HEADER);

	if (p1 == NULL || p2 == NULL || p1 > p2) {
		//PRINTF_ERR("Function Not found Dest Str! \n");
		return -1;
	} else {
		p1 += strlen(HTTP_DIGEST_START_HEADER);

		if (p2 - p1 >= ParserHandleStrLimit) {
			//PRINTF_ERR("ParserHandleStrLimit is not enough! \n");
			return -1;
		}

		memcpy(ParserHandleStr, p1, p2 - p1);
		ParserHandleStr[p2 - p1] = 0;
	}

	return 0;
}

// 计算Http Digest认证后信息发送请求
int HandlerHttpDigestResponStr(char *OnvifHandleStr, char *HandleEndStr,
		int HandleEndStrSize) {
	int Ret = 0;
	int len = 0;

	Ret = ParserOnvifHttpDigest(OnvifHandleStr, HandleEndStr, HandleEndStrSize);
	if (0 == Ret) {
		len = strlen(HandleEndStr) - 4;
		HandleEndStr[len] = 0;
		return 0;
	} else {
		return -1;
	}
}

int GetDigest(char *content, char *OutPutStr, int OutPutStrSize) {
	char *markStrAddr = NULL;
	char *markStrAddrEnd = NULL;
	char *markStr = HTTP_DIGEST_START_HEADER;
	int markStrLen = strlen(markStr);

	markStrAddr = strstr(content, markStr);
	if (NULL == markStrAddr) {
		return -1;
	}
	markStrAddr += markStrLen;

	markStrAddrEnd = strstr(markStrAddr, "\r\n");

	if (NULL == markStrAddrEnd) {
		return -1;
	}

	int OutPutStrLen = markStrAddrEnd - markStrAddr;
	if (OutPutStrLen >= OutPutStrSize) {
		return -1;
	}

	memcpy(OutPutStr, markStrAddr, OutPutStrLen);
	OutPutStr[OutPutStrLen] = 0;

	return 0;
}

