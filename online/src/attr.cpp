#include "attr.h"
#include "dll_iface.h"
#include "global_data.h"

int Attr::get_value_by_type(enum attr_type_t type, uint32_t* value)
{
    attr_ptr_t it = attrs.find(type);
    if (it == attrs.end()){
        return -1;
    }
    *value = it->second;
    return 0;
}

uint32_t Attr::get_value(enum attr_type_t type)
{
    attr_ptr_t it = attrs.find(type);
    if (it == attrs.end()){
		return 0;
    } else {
        return it->second;
    }
}

void Attr::ranged_clear(uint32_t low, uint32_t high, std::vector<uint32_t>& type_list)
{
    if (low > high) {
        return ; 
    }

    attr_ptr_t low_ptr = attrs.lower_bound(low);
    attr_ptr_t high_ptr = attrs.upper_bound(high);

    attr_ptr_t ptr = low_ptr;

    for (ptr = low_ptr; ptr != high_ptr; ptr++) {
        type_list.push_back(ptr->first);
    }

    attrs.erase(low_ptr, high_ptr);
}
