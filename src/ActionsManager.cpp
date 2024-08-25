#include "ActionsManager.hpp"

#include <cstdlib>


static int compActions(const void * a, const void * b) 
{
    return ((Action*)a)->time_min - ((Action*)b)->time_min;
}


void ActionsManager::sortActions()
{
    if(root && actions_in_day){
        Action* data = root;
        for(int d=0, act_num=0; d<WEEK_DAY_NUM; ++d){
            act_num = actions_in_day[d];
            if(act_num){
                std::qsort(data, act_num, sizeof(Action), compActions);
                data += act_num;
            }
        }
    }
}


const Action* ActionsManager::getDayActionList(const int day)const
{
    return root+getDataIndx(day); 
}


int ActionsManager::getDataIndx(const int day)const
{
    int indx = 0;
    if(actions_in_day && root && day < WEEK_DAY_NUM){
        for(int d=0; d<day; ++d)
            indx += actions_in_day[d];
    }
    return indx;
}



size_t ActionsManager::getActionSum(uint8_t *actions_in_day)
{
    size_t sum = 0;
    if(actions_in_day){
        const uint8_t *end = actions_in_day+WEEK_DAY_NUM;
        while(actions_in_day < end)
            sum += *(actions_in_day++);
    }
    return sum;
}


void ActionsManager::clearActionList()
{
    if(root){
        delete [] root;
        root = NULL;
    }
}

bool ActionsManager::isSignal(int day, const int cur_min)
{
    size_t actions_num;
    const Action* day_list;
    bool rele_on = false;
    for(int d=day-1; d >= 0; --d){
        actions_num = getActInDay(d);
        if(actions_num){
            day_list = getDayActionList(d);
            for(int i=actions_num-1; i>=0; --i){
                if(day_list[i].time_min <= cur_min){
                    rele_on = day_list[i].rele_state;
                    break;
                }
            }
            break;
        }
    }
    return rele_on;
}

size_t ActionsManager::getActInDay(const int day)const
{
    if(day<WEEK_DAY_NUM && actions_in_day && root){
        return actions_in_day[day];
    }
    return 0;
}


void ActionsManager::setActionData(Action *actions_data, uint8_t* actions_in_day)
{
    clearActionList();
    this->actions_in_day = actions_in_day;
    this->root = actions_data;
}


