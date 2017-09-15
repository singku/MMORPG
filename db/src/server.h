
#ifndef SERVER_H
#define SERVER_H

extern "C" {
#include <dbser/benchapi.h>
}
#include <dbser/mysql_iface.h>

struct server_config_t
{
    char mysql_host[64];
    char mysql_user[64];
    char mysql_passwd[64];
    int mysql_port;
    char mysql_charset[64];
};

extern "C"
int handle_init(
        int argc, 
        char **argv, 
        int proc_type);

extern "C"
int handle_input(
        const char* recv_buf, 
        int recv_len, 
        const skinfo_t* skinfo);

extern "C"
int handle_process(
        char *recv, 
        int recv_len, 
        char** send_buf, 
        int* send_len, 
        const skinfo_t*);

extern "C"
void handle_fini(
        int proc_type);

extern "C" int handle_filter_key(const char *buf, int len, uint32_t *key);

extern mysql_interface* g_db;

#endif
