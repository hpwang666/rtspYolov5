#ifndef _OSAL_H
#define _OSAL_H

#include "osaltype.h"
#include "tasks.h"
#include "timers.h"
#include "msg.h"

#ifndef NULL
#define NULL (0)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

void osalInitEnv( void );
int osalRunSystem();

int osalAddTask(pTaskEventHandlerFn fn,int flag);
int osalTicksUpdate();

int osalStartTimerEx(int taskID, int event_id, int timeout_value );
int osalClearTaskEvent(int task_id,int eventFlag);
int osalGetTimeoutEx(int task_id, int event_id );
int osalGetTimerActive();
int osalGetTimerCache();
int osalErr(const char *err);
int osalFree(void);
int osalPushMsg(int task_id, char *msg) ;
msg_t osalPullMsg(int task_id);

#ifdef __cplusplus
}
#endif

#endif
