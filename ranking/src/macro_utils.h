#ifndef __MACRO_UTILS_H__
#define __MACRO_UTILS_H__

#define GET_A(type) \
    AttrUtils::get_attr_value(player, type)

#define SET_A(type, value) \
    AttrUtils::set_single_attr_value(player, type, value)

#define ADD_A(type, value) \
    AttrUtils::add_attr_value(player, type, value)

#define SUB_A(type, value) \
    AttrUtils::sub_attr_value(player, type, value)

//idx从0开始
#define SET_BIT(player, type, idx) do { std::bitset<32> bit(GET_A(type)); bit.set(idx);SET_A(type, bit.to_ulong());} while (0) 

#define INCR_A_TO(player, type, value) do { if (GET_A(type) < value) { SET_A(type, value);}} while (0)

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

#define RETURN_ERR(func) do { int ret = func; if (ret) return send_default_cmd_err(player, ret);} while(0)

#define PARSE_RANK_MSG  \
    do {\
        cli_in_.Clear();\
        if (!cli_in_.ParseFromString(req_body)) {\
            ERROR_TLOG("parse err");\
            return -1;\
        }\
        std::string name = cli_in_.GetTypeName(); \
        std::string debug_str = cli_in_.Utf8DebugString();\
        TRACE_TLOG("PARSE MSG:'%s' ok\nMSG:\n[%s]",\
                name.c_str(), debug_str.c_str());\
    } while(0) \

#define RETURN_RANK_MSG \
    do { \
	    cli_out_.SerializeToString(&ack_body); \
        std::string name = cli_out_.GetTypeName(); \
        std::string debug_str = cli_out_.Utf8DebugString();\
        TRACE_TLOG("PARSE MSG:'%s' ok\nMSG:\n[%s]",\
                name.c_str(), debug_str.c_str());\
        return 0; \
    }while(0)
    
#endif
