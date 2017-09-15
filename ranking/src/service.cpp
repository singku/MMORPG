#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "service.h"
#include "redis_service.h"
#include "proto_processor.h"

const char *version_str = "seer_redis-20130104";

enum 
{
    PROC_MAIN = 0,
    PROC_CONN,
    PROC_WORK,
    PROC_TIME 
};

ProtoProcessor *processor;

int handle_init (int argc, char **argv, int pid_type)
{
#ifdef ENABLE_TRACE_LOG
#ifdef USE_TLOG
	SET_LOG_LEVEL(tlog_lvl_trace);
	SET_TIME_SLICE_SECS(86400);
#endif
#endif
	processor = new ProtoProcessor();
	switch (pid_type)
    {
		case PROC_MAIN:
			boot_log(0,0,"\033[1m\033[41mseer redis VERSION:%s\033[0m",version_str);
			return 0;
		case PROC_WORK:
			try {
				connect_redis();
			} catch (redis::connection_error& e) {
				boot_log(0,0,"\033[1m\033[41mfailed to connect to redis:%s\033[0m", e.what());
				return -1;
			}
			init_cmd_procs();
			return 0;
		case PROC_CONN:
			return 0;
		default:
			ERROR_LOG ("invalid pid_type=%d", pid_type);
			return -1;
	}

	return -1;
}

int handle_input (const char* buffer, int length, const skinfo_t *sk)
{
	return processor->get_pkg_len(buffer, length);
}

int handle_process (char *recvbuf, int rcvlen, char **sendbuf, int *sndlen, const skinfo_t *sk)
{
	int ret = processor->process(recvbuf, rcvlen, sendbuf, sndlen);

	return ret;
}

int handle_open (char **buf, int *len, const skinfo_t* sk)
{
    return 0;
}

int handle_close (const skinfo_t* sk)
{
    return 0;
}

void handle_fini (int t)
{
}
