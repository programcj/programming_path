#include <stdio.h>
#include <stdint.h>
#include <openssl/md5.h>

//计算文件内容中index开始的size大小的md5
int md5sum_fileindex(const char *file, long int index, long int size,
		char md5str[33]) {
#define MAXDATABUFF 1024

	MD5_CTX md5_ctx;
	uint8_t md5res[16];
	uint8_t buff[MAXDATABUFF];
	int ret = 0;

	FILE *fp = fopen(file, "rb");
	if (!fp)
		return -1;

	fseek(fp, index, SEEK_SET);
	if (ftell(fp) != index) { //文件长度不足
		fclose(fp);
		return -1;
	}

	memset(md5res, 0, sizeof(md5res));
	MD5_Init(&md5_ctx);

	while (!feof(fp) && size > 0) {
		ret = fread(buff, 1, sizeof(buff), fp);
		if (ret > 0) {

			if (size > ret) {
				size -= ret;
			} else {
				ret = size;
				size = 0;
			}
			MD5_Update(&md5_ctx, buff, ret);   //将当前文件块加入并更新MD5
		}
	}
	fclose(fp);

	if (size != 0) {
		printf("err:file seek end need %ld byte\n", size);
		return -1;
	}

	MD5_Final(md5res, &md5_ctx);  //获取MD5

//MD5(md5str,33,md5res);    //获取字符串MD5
	for (ret = 0; ret < 16; ret++, md5str += 2)
		sprintf(md5str, "%02x", md5res[ret]);

	return 0;
}