#ifndef __CLIENT_H__
#define __CLIENT_H__


#include "common.h"

#ifdef ASSERVER
struct cli_proto_header_t
{
    uint32_t len;
    uint32_t seq;
    uint16_t cmd;
    uint32_t ret;
    uint32_t uid;
    uint32_t u_create_tm;
    uint32_t cb_uid;
    uint32_t cb_u_create_tm;
} __attribute__((packed));
#else
struct cli_proto_header_t
{
    uint32_t len;
    uint16_t head_len;
    uint16_t cmd;
    uint32_t check_sum;
} __attribute__((packed));
#endif


extern const int kBlockSize;
using google::protobuf::Message;


struct Buffer {
public:
	Buffer() : recvlen(0), sendlen(0) {
			recvbuf_len = kBlockSize;
			sendbuf_len = kBlockSize;
			recvbuf = (char *)malloc(kBlockSize);
			sendbuf = (char *)malloc(kBlockSize);
		}

	void Clear(void) {
		recvlen = sendlen = 0;
	}

public:
	int32_t recvlen;
	int32_t sendlen;
	int32_t recvbuf_len;
	int32_t sendbuf_len;
	char *recvbuf;
	char *sendbuf;
};

class Client {
public:
	~Client();
	Client(uint32_t uid, int max_pkg_len);

public:
	inline bool is_connected(void);
	inline bool try_connect_to_peer(const std::string &svraddr, std::ostringstream &err);
	inline void close_connection(void);
	const Buffer &buffer(void) { return buffer_; }
	void clear_buffer(void) { buffer_.Clear(); }

public:
    uint32_t uid() {
        return uid_;
    }
	bool send_msg(Message &msg, std::ostringstream &err);
	bool recv_msg(Message **msg, bool &complete, std::ostringstream &err); 
	bool encode(Message *msg, std::string &pkg); 
	bool decode(const std::string &pkg, Message **msg); 
	int tcp_connect(const std::string &svraddr, std::ostringstream &err);
	int net_tcp_send(std::ostringstream &errmsg);
	int net_tcp_recv(std::ostringstream &errmsg);
    void set_session(string session) {
        session_ = session;
    }
    string get_session() {
        return session_;
    }
    uint32_t walk_idx;

private:
	int parse_sockaddr(const char *str, struct sockaddr *out, int *outlen);
	inline int set_fd_nonblock(int s);
	inline int set_tcp_nodelay(int s);
	inline int calc_buffer_size(int needsize);
    

private:
    uint32_t uid_;
	int connfd_;
	std::string peer_addr_;
	int32_t max_pkg_len_;
	Buffer buffer_;
    string session_;
};



#include "client-inl.h"

extern uint32_t echo_msg;

#endif // __CLIENT_H__
