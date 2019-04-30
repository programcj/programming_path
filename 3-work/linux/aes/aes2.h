/*
 * cyrpt_aes.h
 *
 *  Created on: 2017年11月7日
 *      Author: cj
 */

#ifndef AES_2_H
#define AES_2_H

#include <stdint.h>

/**
 * PKCS7 加密数据长度，补齐位计算，加密总长度-原始数据长度
 */
int crypt_aes_pkcs7_length(int inlen);

int crypt_aes_pkcs7_decode_paddinglen(void *data, int dlen);

/**
 * @in: 需要加密的补齐数据长度
 */
void crypt_aes_encrypt_pkcs7(const void *in, void *out, int iolen, const unsigned  char key[16],
		const unsigned char _iv[16]);

void crypt_aes_decrypt_pkcs7(const void *in, void *out, int iolen, const unsigned  char key[16],
		const unsigned char _iv[16]);

int crypt_aes_encrypt_pkcs7_ex(void *in, int inlen, unsigned char **out,
		int *olen, const unsigned char key[16], const unsigned char _iv[16]);


#endif /* FILE_SRC_INCLUDE_CRYPT_AES_H_ */
