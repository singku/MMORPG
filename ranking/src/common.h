#ifndef COMMON_H_
#define COMMON_H_

#include <inttypes.h>
#include <string>
#include <cassert>
#include <boost/lexical_cast.hpp>
extern "C" {
#include <libtaomee/log.h>
}

#include "proto/common/svr_proto_header.h"
#include "proto/ranking/rank_cmd.h"
#include "proto/ranking/rank.pb.h"
#include "proto/ranking/rank_errno.h"
#include "proto/client/pb0x02.pb.h"

#define PROTO_MAX_SIZE 65536

typedef uint32_t userid_t;

const int SUCC = 0;
const uint32_t nick_size = 64;

int set_std_buf(char** sendbuf, int* sndlen, svr_proto_header_t* rcv_head, int ret, uint32_t private_size);
int set_std_err_buf(char** sendbuf, int* sndlen, svr_proto_header_t* rcv_head, int ret);


template <typename T>
T string_to_number(const std::string& s)
{
	T n = T();
	try {
		n = boost::lexical_cast<T>(s);
	} catch (boost::bad_lexical_cast& e) {
		ERROR_LOG("bad lexical cast: %s, string:%s", e.what(), s.c_str());
	}
	return n;
}

template <typename T>
std::string number_to_string(T n)
{
	std::string s;
	try {
		s = boost::lexical_cast<std::string>(n);
	} catch (boost::bad_lexical_cast& e) {
		ERROR_LOG("bad lexical cast: %s, number:%d", e.what(), static_cast<int>(n));
	}
	return s;
}

template <typename T>
inline void pack_to_string(std::string& s, T& data)
{
	s.append(reinterpret_cast<char*>(&data), sizeof(data));
}

inline void pack_to_string(std::string& s, char* data, size_t len)
{
	s.append(data, len);
}

template <typename T>
inline void unpack_from_string(std::string& s, T& data)
{
	assert(s.size() >= sizeof(data));
	data = *reinterpret_cast<const T*>(s.c_str());
	s.erase(0, sizeof(data));
}

inline void unpack_from_string(std::string& s, char* data, size_t len)
{
	assert(s.size() >= len);
	memcpy(data, s.c_str(), len);
	s.erase(0, len);
}

#define FOREACH(container, it) \
    for(typeof((container).begin()) it=(container).begin(); it!=(container).end(); ++it)

#endif
