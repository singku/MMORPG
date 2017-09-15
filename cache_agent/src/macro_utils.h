#ifndef __MACRO_UTILS_H__
#define __MACRO_UTILS_H__

#define FOREACH(container, it) \
    for(typeof((container).begin()) it=(container).begin(); it!=(container).end(); ++it)

#define REVERSE_FOREACH(container, it) \
    for(typeof((container).rbegin()) it=(container).rbegin(); it!=(container).rend(); ++it)

#define FOREACH_NOINCR_ITER(container, it) \
       for(typeof((container).begin()) it=(container).begin(); it!=(container).end();) 

#define REVERSE_FOREACH_NOINCR_ITER(container, it) \
        for(typeof((container).rbegin()) it=(container).rbegin(); it!=(container).rend();)

//必须在setup_timer之后才能使用
#define NOW()   (get_now_tv()->tv_sec)

#endif
