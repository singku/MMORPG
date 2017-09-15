#include <stdlib.h>
#include <string.h>
#include "mencrypt.h"

static inline u8 GetKeyNum(const u8 * key, int len, int * index) 
{
	if((*index) == len)
	{ 
		(*index) = 0;
		return key[(*index)];
	}
	else
	{ 
		return key[(*index)++];
	}  
}

u8* do_MDecrypt_x86_32 (u8 * bytes_in, int bytes_in_len, const u8 * key, int key_len, int * bytes_out_len)
{
	int key_index = 0;
	int i =0;
	*bytes_out_len = bytes_in_len-1;
	static u8 bytes_out[online_proto_encrypt_pkgbuf_max_len] = {};
	//memset(bytes_out, 0, sizeof(bytes_out));
	bytes_in_len -= 1;
	for(i=0; i<bytes_in_len; i++)
	{
		bytes_out[i] = (bytes_in[i] >> 5);// & 0x7;
		bytes_out[i] |= (bytes_in[i+1] << 3);// & 0xFF;

	}
	i=0;
	while(i<bytes_in_len)
	{
		bytes_out[i] = bytes_out[i] ^ (GetKeyNum(key, key_len, &key_index));
		i++;
	}
	return bytes_out;
}

u8* do_MEncrypt_x86_32 (u8 * bytes_in, int bytes_in_len, const u8 * key, int key_len, int * bytes_out_len)
{
	int key_index = 0;
	int i=0;
	*bytes_out_len = bytes_in_len+1;
	static u8 bytes_out[online_proto_encrypt_pkgbuf_max_len] = {};
	//memset(bytes_out, 0, sizeof(bytes_out));
	//u8 * bytes_out = (u8 *)malloc(*bytes_out_len);
	while(i<bytes_in_len)
	{
		bytes_out[i] = bytes_in[i] ^ (GetKeyNum(key, key_len, &key_index));
		i++;
	}
	bytes_out[*bytes_out_len-1] = 0;
	for(i=(*bytes_out_len)-2; i>=0; i--)
	{
		bytes_out[i+1] |= (bytes_out[i] >> 3);// & 0x1F;
		bytes_out[i] = (bytes_out[i] << 5);// & 0xFF;
	}
	bytes_out[0] |= 3;
	return bytes_out;
} 

/* @brief 后台解密封装函数
 */
void *msg_decrypt(void *buff_in, uint32_t buff_in_len, uint32_t *buff_out_len)
{
	void *buff_out = (void*)do_MDecrypt_x86_32((u8*)buff_in, (int)buff_in_len, 
            (u8*)encrypt_code, encrypt_code_len, (int*)buff_out_len);
    return buff_out;
}

/* @brief 后台加密封装函数
 */
void *msg_encrypt (void *buff_in, uint32_t buff_in_len, uint32_t *buff_out_len)
{
	void* buff_out = (void*)do_MEncrypt_x86_32((u8*)buff_in, (int)buff_in_len,
            (u8*)encrypt_code, encrypt_code_len, (int*)buff_out_len);
    return buff_out;
}

void MGetCheckCode_x86_32 (const u8 * bytes_in, int bytes_in_len, int checkCodeIndex, u8* checkCodes)
{
	int i=0;
	for(i=0; i<4; i++)
	{
		checkCodes[i] = 0;
	}
	int k = 0;
    
    for(i=checkCodeIndex+4; i<bytes_in_len; i++, k++)
    {
        if(k==4)
            k = 0;
        checkCodes[k] ^= bytes_in[i];
    }
}

/** 
 * @brief 包校验和检查
 * 
 * @param bytes_in 
 * @param bytes_in_len 
 * @param checkCodeIndex 
 * 
 * @return 0 失败 1成功
 */
int MCheckCode_x86_32 (const u8 * bytes_in, int bytes_in_len, int checkCodeIndex)
{
	u8 checkCodes[4];
	u8 readCheckCodes[4];
	MGetCheckCode_x86_32(bytes_in, bytes_in_len, checkCodeIndex, checkCodes);
	int i=0;
	int k=0;
	for(i=checkCodeIndex; i<checkCodeIndex+4; i++, k++)
	{
		readCheckCodes[k] = bytes_in[i];
	}
	int result = 1;
	for(i=0; i<4; i++){
		if(readCheckCodes[i] != checkCodes[i])
		{
			result = 0;
			break;
		}
	}
	return result;
}
