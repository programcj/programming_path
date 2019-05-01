/*
 * cyrpt_aes.c
 *
 *  Created on: 2017年11月7日
 *      Author: cj
 */

#include "aes2.h"

#ifdef CONFIG_HAVE_OPENSSL
#include <openssl/aes.h>
#else
#include "aes.h"
#define AES_BLOCK_SIZE 16
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "clog.h"

#define AES_SIZE_BIT	128

int crypt_aes_pkcs7_length(int inlen) {
	if (inlen % AES_BLOCK_SIZE == 0)
		return inlen + 16;
	return ((inlen) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE + AES_BLOCK_SIZE;
}

int crypt_aes_pkcs7_decode_paddinglen(void *data, int dlen) {
	unsigned char *end = ((unsigned char *) data) + (dlen - 1);  //0->len
	unsigned char *tmp = end;

	while (tmp != data && *tmp == *end && (end - tmp) < *end) {
		tmp--;
	}
	return end - tmp != *end ? 0 : *end;
}

void crypt_aes_encrypt_pkcs7(const void *in, void *out, int iolen, const unsigned char key[16],
		const unsigned char _iv[16]) {
	unsigned char keys[AES_SIZE_BIT / 8 + 1];
	unsigned char iv[AES_BLOCK_SIZE]; //加密的初始化向量

	memset(keys, 0, sizeof(keys));
	memcpy(keys, key, 16);
	memcpy(iv, _iv, sizeof(iv));

#ifdef CONFIG_HAVE_OPENSSL
	AES_KEY aes;

	AES_set_encrypt_key(keys, AES_SIZE_BIT, &aes);
	AES_cbc_encrypt((const unsigned char *) in, (unsigned char *) out, iolen,
			&aes, iv,
			AES_ENCRYPT);
#else
	struct AES_ctx ctx;
	AES_init_ctx_iv(&ctx, key, iv);
	memcpy(out, in, iolen);
	AES_CBC_encrypt_buffer(&ctx, out, iolen);
#endif
}

void crypt_aes_decrypt_pkcs7(const void *in, void *out, int iolen, const unsigned char key[16],
		const unsigned char _iv[16]) {
	unsigned char keys[AES_SIZE_BIT / 8 + 1];
	unsigned char iv[AES_BLOCK_SIZE]; //加密的初始化向量

	memset(keys, 0, sizeof(keys));
	memcpy(keys, key, 16);
	memcpy(iv, _iv, sizeof(iv));

#ifdef CONFIG_HAVE_OPENSSL
	AES_KEY aes;

	AES_set_decrypt_key(keys, AES_SIZE_BIT, &aes);  //AES_SIZE_BIT: 128/256
	AES_cbc_encrypt(in, out, iolen, &aes, iv, AES_DECRYPT);//len: %16==0, iv-len=AES_BLOCK_SIZE=16
#else
	struct AES_ctx ctx;
	AES_init_ctx_iv(&ctx, key, iv);
	memcpy(out, in, iolen);
	AES_CBC_decrypt_buffer(&ctx, out, iolen);
#endif
}

int crypt_aes_encrypt_pkcs7_ex(void *in, int inlen, unsigned char **out, int *olen,
		const unsigned char key[16], const unsigned char _iv[16]) {
	unsigned char *encodes = NULL;
	unsigned char *in_cache = NULL;
	int enlen = 0;
	int i = 0;
	int pkcs = 0;

	enlen = crypt_aes_pkcs7_length(inlen);
	pkcs = enlen - inlen;

	in_cache = (unsigned char *) malloc(enlen);
	if (in_cache == NULL)
		return -1;
	encodes = (unsigned char *) malloc(enlen);
	if (encodes == NULL) {
		free(in_cache);
		return -1;
	}

	memcpy(in_cache, in, inlen);
	for (i = inlen; i < enlen; i++)
		in_cache[i] = pkcs;

	*out = encodes;
	*olen = enlen;
	crypt_aes_encrypt_pkcs7(in_cache, encodes, enlen, key, _iv);
	free(in_cache);
	return 0;
}

#if 0
aes_encode(const void *in, int inlen, unsigned char **out, int *outlen,
		unsigned char key[16], unsigned char iv[16]) {
	unsigned char *indata = NULL;
	unsigned char *encodes = NULL;
	int encodeslen = 0;
	unsigned short crc16 = 0;

	indata = (unsigned char *) malloc(inlen );
	if (indata == NULL)
	return -1;

	if (0 == crypt_aes_encrypt_pkcs7_ex(indata, inlen , &encodes, &encodeslen, key, iv)) {
		*out = encodes;
		*outlen = encodeslen;
		free(indata);
		return 0;
	}
	free(indata);
	return -1;
}

aes_decode(const void *in, int inlen, unsigned char **out, int *outlen,
		unsigned char key[16], unsigned char iv[16]) {
	unsigned char *decodes = NULL;
	int decodeslen = 0;
	int rc = 0;

	decodes = (unsigned char *) malloc(inlen + 16);
	if (decodes == NULL)
	return -1;

	crypt_aes_decrypt_pkcs7(in, decodes, inlen, key, iv);
	decodeslen = inlen - crypt_aes_pkcs7_decode_paddinglen(decodes, inlen);

	*out = decodes;
	decodes[decodeslen] = 0;
	*outlen = decodeslen;
	return 0;
}

#endif
