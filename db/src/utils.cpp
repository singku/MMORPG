#include "utils.h"
extern "C" {
#include <libtaomee/log.h>
}

static char hex_buf[65536];
std::string Utils::to_string(uint32_t n)
{
	snprintf(hex_buf, sizeof(hex_buf), "%u", n);
	return std::string(hex_buf);

}
std::string Utils::to_string(int n)
{
	snprintf(hex_buf, sizeof(hex_buf), "%d", n);
	return std::string(hex_buf);                                                                                  

}

void Utils::print_message(const google::protobuf::Message& message)
{
	std::string name = message.GetTypeName();
	std::string debug_str = message.Utf8DebugString();
	TRACE_TLOG("PARSE MSG: '%s' ok\nMSG:\n[%s]",
			name.c_str(), debug_str.c_str());
}
