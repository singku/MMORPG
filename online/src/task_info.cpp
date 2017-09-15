#include <algorithm>
#include "task_info.h"
#include "item.h"
#include "global_data.h"



TaskInfo::TaskInfo()
{

}

TaskInfo::~TaskInfo()
{

}

//将任务信息加入到任务map中
int TaskInfo:: put_task(const task_t& task_info)   
{
   TaskInfoMap::iterator it = task_infos_.find(task_info.task_id);
	//如果没有该任务，则加入到map中
    if (it == task_infos_.end()) {
		task_infos_[task_info.task_id] = task_info;

    } else {
	//由于此函数目前只在接任务时用到，所以这里暂时为空
    }
    
    return 0;
}

//拉取所有已接取的任务
int TaskInfo::get_all_tasks(std::vector<task_t>& task_list) 
{
	for(std::map<uint32_t, task_t>::const_iterator it=task_infos_.begin(); it!=task_infos_.end(); ++it ) {
		task_list.push_back(it->second);
	}
    return 0;
}

//获取已接取的任务的任务id列表  
int TaskInfo::get_all_tasks_id_list(std::vector<uint32_t>& task_id_list) 
{    
    TaskInfoMap::iterator it_task;
    for (it_task = task_infos_.begin(); it_task != task_infos_.end(); it_task++) {
        task_id_list.push_back(it_task->first);
    }
    return 0;
}

//获取已完成的任务的id列表
int TaskInfo::get_finish_task_id_list(std::vector<uint32_t>& task_id_list)
{
    TaskInfoMap::iterator it_task;
    for (it_task = task_infos_.begin(); it_task != task_infos_.end(); it_task++) {
		if (it_task->second.bonus_status == kTaskBonusAccept) {
			task_id_list.push_back(it_task->first);
			break;
		}
    }
    return 0;
}

void TaskInfo::set_task(const task_t& task_data)
{
    // 不存在时会插入新记录
    task_infos_[task_data.task_id] = task_data;
}

void TaskInfo::del_task(uint32_t task_id)
{
    task_infos_.erase(task_id);    
}

bool TaskInfo::is_in_task_list(uint32_t task_id)
{
    TaskInfoMap::iterator it = task_infos_.find(task_id);

    if (it == task_infos_.end()) {
		return false;
    }
    return true;
}

bool TaskInfo::is_in_task_list(uint32_t task_id, uint32_t pro_id)
{
    TaskInfoMap::iterator it = task_infos_.find(task_id);
    if (it == task_infos_.end())
        return false;

    //ProInfoMap::iterator iter = it->second.find(pro_id);
    //if (iter == it->second.end())
    //    return false;

    return true;
}

int TaskInfo::get_task(uint32_t task_id, std::vector<task_t>& task_list)
{
	std::map<uint32_t, task_t>::const_iterator it_task = task_infos_.find(task_id);
	if(it_task != task_infos_.end()) {
		task_list.push_back(it_task->second);
	}
    return 0;
}
