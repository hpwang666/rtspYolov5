#include "stdio.h"
#include "stdlib.h"
#include "tasks.h"

int setTaskEvent( taskList_t taskList,int task_id, int event_flag )
{
	osalTask_t taskSrch = taskList->head;
	while(taskSrch)
	{
		if(taskSrch->id == task_id){
			taskSrch->eventFlag |= event_flag;
			return 0;
		}
		taskSrch=taskSrch->next;
	}
	return 1;
}


int clearTaskEvent( taskList_t taskList,int task_id, int event_flag )
{
	osalTask_t taskSrch = taskList->head;
	
	while(taskSrch)
	{
		if(taskSrch->id == task_id){
			taskSrch->eventFlag &= ~event_flag;
			return 0;
		}
		taskSrch=taskSrch->next;
	}
	return 1;
}


taskList_t taskListInit( void )
{
	taskList_t q;
    q = (taskList_t) calloc(1,sizeof(struct taskList_st));
	q->head=NULL;
	q->taskNum=0;
    return q;
}

int addTask(taskList_t taskList,pTaskEventHandlerFn fn,int flag)
{
	osalTask_t taskSrch=taskList->head;
	osalTask_t q = (osalTask_t) calloc(1,sizeof(struct osalTask_st));
	q->id = taskList->taskNum;
	q->eventFlag = flag;
	q->handle = fn;
	q->next = NULL;
	
	taskList->taskNum++;
	if(taskSrch == NULL){
		taskList->head = q;
		return q->id;
	}
	
	while(taskSrch->next)
	{
		taskSrch = taskSrch->next;
	}
	taskSrch->next = q;
	return q->id;
}

#if 0
int testTask(taskList_t taskList,int task_id, int event_flag)
{
	osalTask_t taskSrch = taskList->head;
	//osalTimeUpdate();
	
	while(taskSrch)
	{
		if(taskSrch->id ==task_id ){
			taskSrch->handle(taskList,taskSrch->id,event_flag);
			break;
		}
		taskSrch=taskSrch->next;
	}
	return taskSrch->id;
}

#endif
