#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#undef  _DEBUG
//#define _DEBUG
#ifdef _DEBUG
	#define debug(...) printf(__VA_ARGS__)
#else
	#define debug(...)
#endif

#include "msg.h"

msgBox_t msgBoxInit(void) 
{
    msgBox_t q;
    q = (msgBox_t) calloc(1,sizeof(struct msgBox_st));
	q->head=NULL;
	q->tail=NULL;
	q->cache = NULL;
	q->activesize = 0;
	q->cachesize =0;
    return q;
}

int pushMsg(msgBox_t msgBox,taskList_t taskList,int task_id, char *msg) 
{
   msg_t newMsg;
   if(msgBox->cache){
		newMsg= msgBox->cache;
		msgBox->cache = msgBox->cache->next;
		msgBox->cachesize--;
		debug("found in msgcache\r\n");
	}
	else {newMsg = (msg_t)calloc(1, sizeof( struct msg_st ) );
		debug("found in calloc\r\n");
	}
	msgBox->activesize++;	
	
     // Fill in new msg
    newMsg->id = task_id;
    memcpy(newMsg->msg,msg,64);
    
	newMsg->next = NULL;
	newMsg->prev = NULL;
	
    if(msgBox->head == NULL)//also tail is NULL 
	{
		msgBox->head=newMsg;
		msgBox->tail=newMsg;
	}
	else
	{
		msgBox->tail->next = newMsg; //add the new to the tail
		newMsg->prev = msgBox->tail;
	
		msgBox->tail = newMsg;
	}
	setTaskEvent( taskList,task_id, OSALMSG );
	return 0;
}

msg_t pullMsg(msgBox_t msgBox,int task_id) 
{
	msg_t srchMsg;
	
	// Head of the timer list
	srchMsg = msgBox->head;

	// Stop when found or at the end
	while ( srchMsg )
	{
		if (srchMsg->id == task_id){
			//prevMsg = srchMsg->next;
			
			//repair the list
			if(srchMsg->prev !=NULL)//if head
				srchMsg->prev->next = srchMsg->next;
			else msgBox->head = srchMsg->next;
			
			if(srchMsg->next!=NULL)//if tail
				srchMsg->next->prev = srchMsg->prev;
			else msgBox->tail = srchMsg->prev;
			
			srchMsg->next = msgBox->cache;
			srchMsg->prev = NULL;
			msgBox->cache = srchMsg; //into cache
			msgBox->cachesize++;
			msgBox->activesize--;
			return srchMsg ; 
		}
		else srchMsg = srchMsg->next;
         
	}
  
	return ( srchMsg );
}
