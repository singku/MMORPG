/*-
 * Copyright (c) 2013 Taomee Co.
 * All rights reserved.
 * 协议加密算法库
 */

#ifndef _MENCRYPT_
#define _MENCRYPT_

#include <stdint.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef char i8;
typedef short i16;
typedef int i32;

#define online_proto_encrypt_pkgbuf_max_len	(1024 * 1024+1)
extern const char *encrypt_code;
extern uint32_t encrypt_code_len;

void *msg_decrypt(void *buff_in, uint32_t buff_in_len, uint32_t *buff_out_len);
void *msg_encrypt(void *buff_in, uint32_t buff_in_len, uint32_t *buff_out_len);

void MGetCheckCode_x86_32 (const u8 * bytes_in, int bytes_in_len, int checkCodeIndex, u8* checkCodes);
int MCheckCode_x86_32 (const u8 * bytes_in, int bytes_in_len, int checkCodeIndex);

#endif
