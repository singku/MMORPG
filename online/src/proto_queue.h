
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
        fdsession_t* fdsession; // 客户端session 
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

    inline int push_proto(const char* data, int data_len, fdsession_t* fdsession)
    {
        if (full()) {
            return -1; 
        }

        proto_t proto = {0};
        proto.len = data_len;
        proto.data = (char *)malloc(data_len);
        proto.fdsession = fdsession;
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

    // 清除指定fd的协议缓存，手机端可能一个玩家同时有多个fd
    inline void clean_proto_by_fd(int fd)
    {
        std::queue<ProtoQueue::proto_t> tmp_queue;

        // 将相同fd的proto移除释放，不同fd的放入临时队列中暂存
        while (!queue_.empty()) {
            ProtoQueue::proto_t proto = queue_.front(); 
            queue_.pop();

            if (proto.fdsession->fd == fd) {
                free(proto.data);
            } else {
                tmp_queue.push(proto); 
            }
        }

        // 临时队列中的暂存协议导入到原来队列中
        while (!tmp_queue.empty()) {
            ProtoQueue::proto_t proto = tmp_queue.front();    
            tmp_queue.pop();

            queue_.push(proto);
        }
    }

private:
    std::queue<ProtoQueue::proto_t> queue_;
    uint32_t max_num_;
};

#endif
