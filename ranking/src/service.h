#ifndef SERVICE_H_
#define SERVICE_H_

#include "common.h"
//#include "../include/benchapi.h"

#include <stdint.h>

struct skinfo_t;

__BEGIN_DECLS

int handle_init (int, char **, int);
int handle_input (const char*, int, const skinfo_t*);
int handle_filter_key (const char*, int, uint32_t*);
int handle_process (char *, int , char **, int *, const skinfo_t*);
int handle_open (char **, int *, const skinfo_t*);
int handle_close (const skinfo_t*);
int handle_timer (int *);
void handle_fini (int);

__END_DECLS

class ProtoProcessor;

extern ProtoProcessor *processor;
    
#endif
