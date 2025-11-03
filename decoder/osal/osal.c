#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "osal.h"
#include <pthread.h>

static osalEnv_t osalEnv;


void osalInitEnv(void)
{
	osalEnv=(osalEnv_t) calloc(1,sizeof(struct osalEnv_st));
	osalEnv->timerList = timerListInit();
	osalEnv->taskList= taskListInit();
	osalEnv->msgBox = msgBoxInit();
	pthread_mutex_init(&(osalEnv->lock), NULL);
}


int osalRunSystem()
{
	
	osalTask_t taskSrch = osalEnv->taskList->head;
	osalTicksUpdate();
	while(taskSrch)
	{
		if(taskSrch->eventFlag){
			//printf("osal[%d][%d]\r\n",taskSrch->id,taskSrch->eventFlag);
			taskSrch->eventFlag= taskSrch->handle(taskSrch->id,taskSrch->eventFlag);
		}
		taskSrch=taskSrch->next;
	}
	
	return 0;
}

int osalAddTask(pTaskEventHandlerFn fn,int flag)
{
	int id = addTask(osalEnv->taskList, fn, flag);
	return id;
}

int osalTicksUpdate()
{
	pthread_mutex_lock(&(osalEnv->lock)); 
	timerUpdate( osalEnv->timerList ,osalEnv->taskList);
	pthread_mutex_unlock(&(osalEnv->lock));
	return 0;
}

int osalStartTimerEx(int taskID, int event_id, int timeout_value )
{
	pthread_mutex_lock(&(osalEnv->lock)); 
	if((taskID) >= osalEnv->taskList->taskNum) {osalErr("not find task id");return 1;}
	startTimerEx(osalEnv->timerList,taskID,event_id,timeout_value);
	pthread_mutex_unlock(&(osalEnv->lock));
	return 0;
	
}

 int osalClearTaskEvent(int task_id,int eventFlag)
{
	pthread_mutex_lock(&(osalEnv->lock)); 
	clearTaskEvent(osalEnv->taskList,task_id,eventFlag);
	stopTimerEx(osalEnv->timerList,task_id,eventFlag);//还未执行的也要清除
	pthread_mutex_unlock(&(osalEnv->lock));
	return 0;
} 


int osalGetTimeoutEx(int task_id, int event_id )
{
	return getTimeoutEx(osalEnv->timerList,task_id,event_id);
}


int osalGetTimerActive()
{
	return getTimerActive(osalEnv->timerList);
}

int osalGetTimerCache()
{
	return getTimerCache(osalEnv->timerList);
}

int osalPushMsg(int task_id, char *msg)
{
	int res = pushMsg(osalEnv->msgBox,osalEnv->taskList,task_id, msg) ;
	if(res) osalErr("msg too long");
	return res;
}
msg_t osalPullMsg(int task_id)
{
	return pullMsg(osalEnv->msgBox,task_id) ;
}

int osalErr(const char *err)
{
	osalEnv->err[0]++;
	memcpy(osalEnv->err+1,err,strlen(err));
	return 0;
}

int osalFree(void)
{
	timerRec_t timerRec = osalEnv->timerList->head;
	timerRec_t t;
	osalTask_t osalTask = osalEnv->taskList->head;
	osalTask_t task;
	while(timerRec){
		t=timerRec->next;
		free(timerRec);
		timerRec = t;
	}
	timerRec = osalEnv->timerList->cache;
	while(timerRec){
		t=timerRec->next;
		free(timerRec);
		timerRec = t;
	}
	free(osalEnv->timerList);
	
	
	while(osalTask){
		task=osalTask->next;
		free(osalTask);
		osalTask = task;
	}
	free(osalEnv->taskList);
	pthread_mutex_destroy(&(osalEnv->lock)); 
	return 0;
}
