#ifndef __QUESTION_H_
#define __QUESTION_H_

#include "common.h"
const uint32_t time_key = 13;
enum phase_type{
	null  = 0,
	ready = 1,
	start = 2,
	end   = 3,
};

struct question_phase_t {
	question_phase_t (){
		clear();
	}
	void clear(){
		phase = null;
		time_subkey = 0;
	}
	phase_type  phase;
	uint32_t time_subkey;
};

// question_phase_t system_phase; 
#endif
