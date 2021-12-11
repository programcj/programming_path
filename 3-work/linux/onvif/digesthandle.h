#ifndef __DIGEST_HANDLE_H_
#define __DIGEST_HANDLE_H_

char *CreateRandomNum (int N);
int HttpDigestCreater (char *DisgetStr, char *DisgetResult, int DisgetResultLimit, char *IP, char *username, char *password, char *cnonce, int model);
int HandlerHttpDigestResponStr (char *OnvifHandleStr, char *HandleEndStr, int HandleEndStrSize);
int GetDigest (char *content, char *OutPutStr, int OutPutStrSize);

#endif

