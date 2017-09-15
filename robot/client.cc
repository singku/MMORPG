#include "cli_cmd.h"
#include "client.h"
#include "config.h"
#include "pb_master.h"
#include "mencrypt.h"
#include <libtaomee++/utils/strings.hpp>

using google::protobuf::Reflection;
using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;

uint32_t echo_msg = 0;

const int kBlockSize = 4096;

const char *encrypt_code = "ayxypi_117ATaoMee.co#m8de";
uint32_t encrypt_code_len = strlen(encrypt_code);
bool cli_proto_encrypt = true;

Client::~Client() {
	if (is_connected()) {
		close_connection();
	}
	if (buffer_.recvbuf) {
		free(buffer_.recvbuf);
		buffer_.recvbuf = 0;
	}
	if (buffer_.sendbuf) {
		free(buffer_.sendbuf);
		buffer_.sendbuf = 0;
	}
	buffer_.recvlen = buffer_.sendlen = buffer_.recvbuf_len = buffer_.sendbuf_len = 0;
}

Client::Client(uint32_t uid, int max_pkg_len)
	: uid_(uid),
      connfd_(-1),
	  max_pkg_len_(max_pkg_len) { walk_idx = 0;}

bool Client::send_msg(Message &msg, std::ostringstream &err) {
	if (!is_connected()) {
		err << "Not connecting to peer: " << peer_addr_
			<< ", when sending msg: " << msg.GetTypeName();
		return false;
	}

	std::string pkg;
	if (!encode(&msg, pkg)) {
		err << "failed send: err encode: " << msg.GetTypeName();
		return false;
	}
	if (static_cast<int32_t>(pkg.size()) > max_pkg_len_) {
		err << "failed send: too big msg: " << msg.GetTypeName()
			<< ", size=" << pkg.size() << " > max_pkg_len=" << max_pkg_len_;
		return false;
	}

	if (buffer_.sendlen + (int)pkg.size() + 128 >= buffer_.sendbuf_len) { // 快满了, 扩容
		int new_size = calc_buffer_size(buffer_.sendbuf_len + pkg.size());
		buffer_.sendbuf = (char *)realloc(buffer_.sendbuf, new_size);
		buffer_.sendbuf_len = new_size;
	}
	memcpy(buffer_.sendbuf + buffer_.sendlen, pkg.c_str(), pkg.size());
	buffer_.sendlen += pkg.size();

	return true;
}

bool Client::recv_msg(Message **msg, bool &complete, std::ostringstream &err) {
	complete = false;
	if (buffer_.recvlen < 4) {
		return true;
	}
	char *recvbuf = buffer_.recvbuf;
	int len = *((uint32_t *)recvbuf);
	if ((uint32_t)len < sizeof(cli_proto_header_t) || len > max_pkg_len_) {
		err << "failed recv: invalid msg,"
			<< " errlen=" << len << ", not in range ["
            << sizeof(cli_proto_header_t)
            << ","
            << max_pkg_len_ 
            << "]";
		return false;
	}
	if (buffer_.recvlen < len) {
		return true;
	}
	std::string recvstr(recvbuf, len);
	if (!decode(recvstr, msg)) {
		err << "failed recv: err decode msg";
		return false;
	}
	if (buffer_.recvlen == len) {
		buffer_.recvlen = 0;
	} else {
		memmove(buffer_.recvbuf, buffer_.recvbuf + len, buffer_.recvlen - len);
		buffer_.recvlen -= len;
	}
	complete = true;
	return true;
}

bool Client::encode(Message *msg, std::string &pkg) {
	std::string type_name = msg->GetTypeName(); 
    
    if (type_name == "onlineproto.cs_0x0003_create_role") {
        onlineproto::cs_0x0003_create_role *ptr = (onlineproto::cs_0x0003_create_role *)msg;
        ptr->set_session(this->get_session());
    } else if (type_name == "onlineproto.cs_0x0004_get_svr_list") {
        onlineproto::cs_0x0004_get_svr_list *ptr = (onlineproto::cs_0x0004_get_svr_list *)msg;
        ptr->set_session(this->get_session());
    }

    uint32_t cmd = get_cmd_by_msg_name(type_name);
    if (cmd == 0) {
        return false;
    }


    cli_proto_header_t head;
	pkg.clear();

#ifdef ASSERVER
    head.len = sizeof(head) + msg->ByteSize();
    head.seq = uid();
    head.cmd = cmd;
    head.ret = 0;
    head.uid = uid();
    head.u_create_tm = 946656000; // 2000-01-01 00:00:00
    head.cb_uid = uid();
    head.cb_u_create_tm = 946656000;
    pkg.append((char*)(&head), sizeof(head));
    if (!msg->AppendToString(&pkg)) {
        LOG(ERROR) << "Failed encode msg for: " << type_name;
        return false;
    }

#else
    onlineproto::proto_header_t pb_header;
    pb_header.set_uid(uid());
    pb_header.set_seque(0);
    pb_header.set_ret(0);
    uint32_t pb_head_len = pb_header.ByteSize();

    head.len = sizeof(head) + pb_header.ByteSize() + msg->ByteSize();
    head.head_len = pb_header.ByteSize();
    head.cmd = cmd;
    
    if (cli_proto_encrypt) {
        char tmp[1024*1024];
        uint32_t* p_check_sum =reinterpret_cast<uint32_t *>(tmp);
        *p_check_sum = 0;
        pb_header.SerializeToArray(tmp + sizeof(uint32_t), sizeof(tmp));
        msg->SerializeToArray(tmp + sizeof(uint32_t) + pb_head_len, sizeof(tmp) - sizeof(uint32_t) - pb_head_len);
        uint32_t len = pb_head_len + msg->ByteSize() + sizeof(uint32_t);
        uint32_t out_len = 0;
        char bytes_out[online_proto_encrypt_pkgbuf_max_len] = {};
        char *encrypt_part;

        //char *pt = bin2hex(0, (char*)tmp, len);
        //LOG(ERROR) << "Bef Encrypt: " << len << "[" << pt << "]";
        msg_encrypt(tmp, len, bytes_out, &out_len);
        //pt = bin2hex(0, (char*)bytes_out, out_len);
        //LOG(ERROR) << "Aft Encrypt:" << out_len << "[" << pt << "]";

        encrypt_part = bytes_out;
        head.len = sizeof(head) - sizeof(uint32_t) + out_len;
        pkg.append((char*)(&head.len), sizeof(head.len));
        pkg.append((char*)(&head.head_len), sizeof(head.head_len));
        pkg.append((char*)(&head.cmd), sizeof(head.cmd));
        pkg.append(encrypt_part, out_len);

    } else {
        pkg.append((char*)(&head), sizeof(head));
        if (!pb_header.AppendToString(&pkg)) {
            LOG(ERROR) << "Failed encode msg for: " << type_name;
            return false;
        }
        if (!msg->AppendToString(&pkg)) {
            LOG(ERROR) << "Failed encode msg for: " << type_name;
            return false;
        }
    }
#endif

    if (echo_msg)
        LOG(ERROR) << "[ENCODE: pkg_len=" << pkg.size()
            << " bodysize=" << msg->ByteSize() << "]\n"
            << msg->GetTypeName() << "\n"
            << "msg:[\n" << msg->Utf8DebugString() <<"]\n";

	return true;
}

bool Client::decode(const std::string &pkg, Message **msg) {
	*msg = 0;

	int min_totlen = sizeof(cli_proto_header_t);
	int getlen = static_cast<int>(pkg.size());
	if (getlen < min_totlen) {
		LOG(ERROR) << "decode err: getlen(" << getlen << ") < min(" << min_totlen << ")";
		return false;
	}
	// 至此, pkg 至少有 min_totlen 那么长 (保证解包不会越界)

	int32_t tlen_inpkg = *(int32_t *)(pkg.c_str());

	if (tlen_inpkg != getlen) {
		LOG(ERROR) << "decode err: getlen(" << getlen << ") != tlen_inpkg(" << tlen_inpkg << ")"; 
		return false;
	}

	const cli_proto_header_t *head = (cli_proto_header_t*)(pkg.c_str());
    uint32_t cmd = head->cmd;

 	const char *data;
	int32_t datalen;

#ifdef ASSERVER
    if (head->ret) {
        LOG(ERROR) << "Client Recv Server Err: " << head->ret
            << " Cmd: " << cmd;
        return false;
    }
 	data = pkg.c_str() + sizeof(cli_proto_header_t);
	datalen = tlen_inpkg - sizeof(cli_proto_header_t);   

#else

    const char *tmp = 0;
    uint32_t plain_len = 0;
    if (cli_proto_encrypt) {//解包
        uint32_t out_len;
        char bytes_out[online_proto_encrypt_pkgbuf_max_len] = {};
        // checksum参与加解密
        msg_decrypt((char*)pkg.c_str() + sizeof(cli_proto_header_t) - sizeof(uint32_t),
                head->len - sizeof(cli_proto_header_t) + sizeof(uint32_t), bytes_out, &out_len);
        tmp = bytes_out;
        plain_len = out_len;
    } else {
        tmp = pkg.c_str() + sizeof(cli_proto_header_t);
        plain_len = head->len - sizeof(cli_proto_header_t);
    }

    onlineproto::proto_header_t pb_header;
    const char *pb_head_addr = tmp + sizeof(uint32_t);
    if (!pb_header.ParseFromArray(pb_head_addr, head->head_len)) {
		LOG(ERROR) << "decode head err: failed parse pb_header: ";
		return false;
    }
    if (pb_header.ret()) {
        LOG(ERROR) << "Client Recv Server Err: " << pb_header.ret()
                    << " Cmd: " << cmd;
        return false;
    }
 	data = tmp + head->head_len + sizeof(uint32_t);
	datalen = plain_len - head->head_len - sizeof(uint32_t);   
#endif

    string msg_type_name = get_sc_msg_name_by_cmd(cmd);
    if (msg_type_name.size() == 0) {
        return false;
    }
	*msg = PB_MASTER.create_message(msg_type_name);
	if (!(*msg)) {
		LOG(ERROR) << "decode err: failed create_message: " << msg_type_name;
		return false;
	}

	if (!(*msg)->ParseFromArray(data, datalen)) {
		LOG(ERROR) << "decode err: failed parse message: " << msg_type_name
            << " raw_data:len=" << datalen << "[" << data <<"]\n";
		delete *msg;
		*msg = 0;
		return false;
	}

    if (msg_type_name == "onlineproto.sc_0x0001_login") {
        onlineproto::sc_0x0001_login tmp;
        tmp.CopyFrom(**msg);
        if (tmp.has_session()) {
            this->set_session(tmp.session());
        }
    }

    if (echo_msg)
	    LOG(ERROR) << "[DECODE]\n" 
            << msg_type_name << "\n"
            << "msg:[\n"
            << (*msg)->Utf8DebugString()
            << "]\n";

	return true;
}

int Client::parse_sockaddr(const char *str, struct sockaddr *out, int *outlen) {
	int port;
	char buf[128];
	const char *cp, *addr_part, *port_part;
	int is_ipv6;
	/* recognized formats are:
	 * [ipv6]:port
	 * ipv6
	 * [ipv6]
	 * ipv4:port
	 * ipv4
	 */

	cp = strchr(str, ':');
	if (*str == '[') {
		int len;
		if (!(cp = strchr(str, ']'))) {
			return -1;
		}
		len = (int) ( cp-(str + 1) );
		if (len > (int)sizeof(buf)-1) {
			return -1;
		}
		memcpy(buf, str+1, len);
		buf[len] = '\0';
		addr_part = buf;
		if (cp[1] == ':')
			port_part = cp+2;
		else
			port_part = NULL;
		is_ipv6 = 1;
	} else if (cp && strchr(cp+1, ':')) {
		is_ipv6 = 1;
		addr_part = str;
		port_part = NULL;
	} else if (cp) {
		is_ipv6 = 0;
		if (cp - str > (int)sizeof(buf)-1) {
			return -1;
		}
		memcpy(buf, str, cp-str);
		buf[cp-str] = '\0';
		addr_part = buf;
		port_part = cp+1;
	} else {
		addr_part = str;
		port_part = NULL;
		is_ipv6 = 0;
	}

	if (port_part == NULL) {
		port = 0;
	} else {
		port = atoi(port_part);
		if (port <= 0 || port > 65535) {
			return -1;
		}
	}

	if (!addr_part) {
		return -1; /* Should be impossible. */
	}

	if (is_ipv6) {
		struct sockaddr_in6 sin6;
		memset(&sin6, 0, sizeof(sin6));
		sin6.sin6_family = AF_INET6;
		sin6.sin6_port = htons(port);
		if (inet_pton(AF_INET6, addr_part, &sin6.sin6_addr) != 1) {
			return -1;
		}
		if ((int)sizeof(sin6) > *outlen) {
			return -1;
		}
		memset(out, 0, *outlen);
		memcpy(out, &sin6, sizeof(sin6));
		*outlen = sizeof(sin6);
		return 0;
	}

	// ipv4
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	if (inet_pton(AF_INET, addr_part, &sin.sin_addr) != 1) {
		return -1;
	}
	if ((int)sizeof(sin) > *outlen) {
		return -1;
	}
	memset(out, 0, *outlen);
	memcpy(out, &sin, sizeof(sin));
	*outlen = sizeof(sin);
	return 0;
}

int Client::tcp_connect(const std::string &svraddr, std::ostringstream &err) {
	struct sockaddr_in peer;
	int peer_addrlen = sizeof(peer);
	memset(&peer, 0, sizeof(peer));
	if (parse_sockaddr(svraddr.c_str(), (struct sockaddr *)&peer, &peer_addrlen) == -1) {
		err << "invalid peeraddr: " << svraddr;
		return -1;
	}

	int s = socket(PF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		err << "socket: " << strerror(errno);
		return -1;
	}
	if (connect(s, (const sockaddr*)&peer, sizeof(peer)) == -1) {
		err << "connect: " << strerror(errno);
		close(s);
		return -1;
	}
	if (set_fd_nonblock(s) == -1) {
		err << "set_fd_nonblock: " << strerror(errno);
		close(s);
		return -1;
	}
	if (set_tcp_nodelay(s) == -1) {
		err << "set_tcp_nondelay: " << strerror(errno);
		close(s);
		return -1;
	}

	return s;
}

int Client::net_tcp_recv(std::ostringstream &errmsg) {
	int nread = 0;
    while(true) {
        nread = read(connfd_, buffer_.recvbuf + buffer_.recvlen,
				buffer_.recvbuf_len - buffer_.recvlen);
        if (nread == 0) { // EOF
			errmsg << "recv meet EOF (peer shutdown), fd: " << connfd_;
			close(connfd_);
			connfd_ = -1;
			return -1;
		}
        if (nread == -1) {
			if (errno == EINTR) { continue; }
			if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }
			// other err
			errmsg << "recv meet error, fd: " << connfd_
				<< ", err(" << errno << "): " << strerror(errno);
			close(connfd_);
			connfd_ = -1;
			return -1;
		}
        buffer_.recvlen += nread;
		if (buffer_.recvlen + 128 >= buffer_.recvbuf_len) { // 满了, 扩容
			int new_size = calc_buffer_size(buffer_.recvbuf_len + kBlockSize);
			buffer_.recvbuf = (char *)realloc(buffer_.recvbuf, new_size);
			buffer_.recvbuf_len = new_size;
		}
    }
	return 0;
}

int Client::net_tcp_send(std::ostringstream &errmsg) {
	if (buffer_.sendlen == 0) return 0;
	int total_sent = 0;
	int nwritten = 0;
	while(true) {
		nwritten = write(connfd_, buffer_.sendbuf + total_sent, buffer_.sendlen - total_sent);
		if (nwritten == 0) { // EOF
			errmsg << "send meet EOF??? (peer shutdown), fd: " << connfd_;
			close(connfd_);
			connfd_ = -1;
			return -1;
		}
		if (nwritten == -1) {
			if (errno == EINTR) { continue; }
			if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }
			errmsg << "send meet error, fd: " << connfd_
				<< ", err(" << errno << "): " << strerror(errno);
			close(connfd_);
			connfd_ = -1;
			return -1;
		}
		total_sent += nwritten;
		if (total_sent == buffer_.sendlen) { break; }
	}
	if (total_sent == buffer_.sendlen) {
		buffer_.sendlen = 0;
	} else { // total_sent < buffer_.sendlen
		memmove(buffer_.sendbuf, buffer_.sendbuf + total_sent, buffer_.sendlen - total_sent);
		buffer_.sendlen -= total_sent;
	}
	return 0;
}
