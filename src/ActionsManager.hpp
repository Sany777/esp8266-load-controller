#pragma once

#include "stdint.h"
#include <cstring> 
#include <cstddef>


struct Action
{
	int time_min;
	bool rele_state;
    Action():time_min(0),rele_state(false){}
    Action(int time_min, bool rele_state):time_min(time_min), rele_state(rele_state){}
    static int getMin(const Action &ob){ return ob.time_min%60; };
    static int getHour(const Action &ob){ return ob.time_min/60; };
    static int getReleState(const Action &ob){ return ob.rele_state; };
};



class ActionsManager{

private:
    static const int WEEK_DAY_NUM = 7;
    Action *root;
    uint8_t *actions_in_day;

protected:

    void sortActions();
    void setActionData(Action *actions_data, uint8_t* actions_in_day);

public:
    ActionsManager() : root(NULL), actions_in_day(NULL){}
    static size_t getActionSum(uint8_t* actions_in_day);
    const Action* getDayActionList(const int day)const;
    bool isSignal(int day, const int cur_min);
    int getDataIndx(const int day)const;
    void clearActionList();
    size_t getActInDay(const int day)const;
    const Action* getActionsList()const{ return root; };
    size_t getActionSum(){ return root ? getActionSum(this->actions_in_day) : 0; };
    ~ActionsManager(){ clearActionList(); };
};
