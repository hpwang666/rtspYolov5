#ifndef _TASKS_H
#define _TASKS_H

#include "osaltype.h"
#ifndef NULL
#define NULL (0)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

int setTaskEvent( taskList_t taskList,int task_id, int event_flag );
int clearTaskEvent( taskList_t taskList,int task_id, int event_flag );
taskList_t taskListInit( void );
int addTask(taskList_t taskList,pTaskEventHandlerFn fn,int flag);

#ifdef __cplusplus
}
#endif

#endif
