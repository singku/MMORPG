#include <cstdlib>
#include <cstring>
#include "common.h"

char std_send_buf[PROTO_MAX_SIZE];

int set_std_buf(char** sendbuf, int* sndlen, svr_proto_header_t* rcv_head, int ret, uint32_t private_size)
{
	svr_proto_header_t* ph;
	*sndlen = sizeof(svr_proto_header_t) + private_size;
	if (*sndlen > PROTO_MAX_SIZE) {
		return -1;
	}
    *sendbuf = std_send_buf;
	ph = reinterpret_cast<svr_proto_header_t*>(*sendbuf);
	memcpy(ph, rcv_head, sizeof(svr_proto_header_t));
	ph->len = *sndlen;
	ph->ret = ret;
	return 0;
}

int set_std_err_buf(char** sendbuf, int* sndlen, svr_proto_header_t* rcv_head, int ret)
{
	return set_std_buf(sendbuf, sndlen, rcv_head, ret, 0);
}

