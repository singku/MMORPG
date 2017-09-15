
#ifndef INTERFACE_MANAGER_H
#define INTERFACE_MANAGER_H

#include <stdint.h>
#include <map>

template<class T>
class InterfaceManager
{
public:
    ~InterfaceManager()
    {
        for (typeof(ifaces_.begin()) it=ifaces_.begin(); it!=(ifaces_).end(); ++it) {
            delete it->second;
        }

        ifaces_.clear();
    }

    void register_iface(uint32_t type, T* iface)
    {
        ifaces_[type] = iface;
    }

    T* get_iface(uint32_t type)
    {
        if (ifaces_.find(type) == ifaces_.end()) {
            return NULL; 
        } else {
            return ifaces_[type]; 
        }
    }

private:
    std::map<uint32_t, T*> ifaces_;
};

#endif
