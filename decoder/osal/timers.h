#ifndef _TIMERS_H
#define _TIMERS_H

#include "osaltype.h"

#ifndef NULL
#define NULL (0)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

timerList_t timerListInit(void) ;
int addTimer( timerList_t timerList,int task_id, int event_flag, int timeout );
timerRec_t findTimer( timerList_t timerList,int task_id, int event_flag );
int startTimerEx( timerList_t timerList,int taskID, int event_id, int timeout_value );
int stopTimerEx(timerList_t timerList, int task_id, int event_id );
int getTimeoutEx(timerList_t timerList, int task_id, int event_id );
int getTimerActive( timerList_t timerList );
int getTimerCache(timerList_t timerList);
int timerUpdate( timerList_t timerList,taskList_t taskList);

#ifdef __cplusplus
}
#endif

#endif
