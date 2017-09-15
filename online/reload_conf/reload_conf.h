#ifndef __RELOAD_CONF_H__
#define __RELOAD_CONF_H__

#include"common.h"
#include"../src/proto.h"
#include"../../proto/common/svr_common.pb.h"

using namespace std;

#define MAX_PKG_SIZE    (1024)


int encode(const google::protobuf::Message* message, string& send_str)
{
    char buf[4096];
    svr_proto_header_t* header = (svr_proto_header_t *)buf;
    char* body = (buf + sizeof(*header));
    int bodylen = 0;

    if (message) {
        if (!message->SerializeToArray(body, 
                    sizeof(buf) - sizeof(*header))) {
            fprintf(stderr, "serialize failed\n");
            return -1;
        }
        bodylen = message->ByteSize();
        cout << "bodylen:" << bodylen << endl;
    }

    header->len = sizeof(*header) + bodylen;
    header->seq = 0;
    header->cmd = 0;
    header->ret = 0;
    header->uid = 0;
    send_str = buf;

    cout << "send_str:" << send_str << endl;
    return 0;
}


class reload_mgr_t {
public:
    reload_mgr_t() : mcast_fd_(-1), mcast_port_(0) {
        memset(&mcast_addr_, 0, sizeof(mcast_addr_));
        memset(mcast_ip_, 0, sizeof(mcast_ip_));
        memset(out_ip_, 0, sizeof(out_ip_));
        memset(out_iface_, 0, sizeof(out_iface_));
    }

public: 
    bool init(char *mcast_ip, int16_t mcast_port, char *out_ip, char *out_iface);
    bool connect();
    int32_t send(std::string &str);
    int32_t send(const google::protobuf::Message* message);

public:    
    bool is_connected() { return (mcast_fd_ >= 0); }
    
private:
    int mcast_fd_;   
    struct sockaddr_in mcast_addr_;
    char mcast_ip_[16];  //组播ip
    int16_t mcast_port_; //组播port
    
    char out_ip_[16];    //ip for outgoing multicast datagrams
    char out_iface_[16]; //interface on which arriving multicast datagrams will send
    
public:
    char sendbuff[MAX_PKG_SIZE];
    int32_t sendlen;
};


#endif // __RELOAD_CONF_H__
