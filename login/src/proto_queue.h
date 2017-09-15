
#ifndef PROTO_QUEUE_H
#define PROTO_QUEUE_H

#include <stdint.h>
#include <queue>

/**
 * @brief  ProtoQueue 缓存一部分协议
 */
class ProtoQueue
{
public:
    struct proto_t
    {
        int len;
        char* data; 
    };

    ProtoQueue(uint32_t max_num)
    {
        max_num_ = max_num;
    }

    ~ProtoQueue()
    {
        while (!queue_.empty()) {
            ProtoQueue::proto_t proto = queue_.front(); 
            queue_.pop();
            free(proto.data);
        }
    }

    inline bool full()
    {
        return (queue_.size() >= max_num_);
    }

    inline bool empty()
    {
        return queue_.empty(); 
    }

    inline uint32_t size()
    {
        return queue_.size(); 
    }

    inline int push_proto(const char* data, int data_len)
    {
        if (full()) {
            return -1; 
        }

        proto_t proto = {0};
        proto.len = data_len;
        proto.data = (char *)malloc(data_len);
        memcpy(proto.data, data, data_len);

        queue_.push(proto);

        return 0;
    }

    inline int pop_proto(ProtoQueue::proto_t& proto)
    {
        if (queue_.empty()) {
            return -1; 
        }

        proto = queue_.front();
        queue_.pop();

        return 0;
    }

private:
    std::queue<ProtoQueue::proto_t> queue_;
    uint32_t max_num_;
};

#endif
