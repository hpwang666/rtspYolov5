#include "stdlib.h"
#include "stdio.h"
#include "tasks.h"
#include "timers.h"
#include <sys/time.h>


#undef  _DEBUG
//#define _DEBUG
#ifdef _DEBUG
	#define debug(...) printf(__VA_ARGS__)
#else
	#define debug(...)
#endif

static unsigned long long    currentTime;
static void getCurentTime();

timerList_t timerListInit(void) 
{
    timerList_t q;
    q = (timerList_t) calloc(1,sizeof(struct timerList_st));
	q->head=NULL;
	q->tail=NULL;
	q->cache = NULL;
    return q;
}

static void getCurentTime(void)
{
    time_t           sec;
    int       		msec;
    struct timeval   tv;

    gettimeofday(&tv, NULL);
    sec = tv.tv_sec;
    msec = tv.tv_usec / 1000;
	currentTime = sec * 1000LL + msec;
}


int addTimer( timerList_t timerList,int task_id, int event_flag, int timeout )
{
	timerRec_t newTimer;
	
	// Look for an existing timer first
	getCurentTime();
	newTimer = findTimer(timerList,task_id,event_flag);
	if(newTimer) {newTimer->timeout = timeout+currentTime; debug("found old\r\n"); return 1;}
    // New Timer
	if(timerList->cache){
		newTimer= timerList->cache;
		timerList->cache = timerList->cache->next;
		timerList->cachesize--;
		debug("timer found in cache\r\n");
	}
	else {newTimer = (timerRec_t)calloc(1, sizeof( struct timerRec_st ) );
		debug("timer found in calloc\r\n");
	}
	timerList->activesize++;	
     // Fill in new timer
    newTimer->id = task_id;
    newTimer->eventFlag = event_flag;
    newTimer->timeout = timeout+currentTime;
    
	newTimer->next = NULL;
	newTimer->prev = NULL;
	
    if(timerList->head == NULL)//also tail is NULL 
	{
		timerList->head=newTimer;
		timerList->tail=newTimer;
	}
	else
	{
		timerList->tail->next = newTimer; //add the new to the tail
		newTimer->prev = timerList->tail;
	
		timerList->tail = newTimer;
	}
	return 0;
}


timerRec_t findTimer( timerList_t timerList,int task_id, int event_flag )
{
  timerRec_t timerSrch;

  // Head of the timer list
  timerSrch = timerList->head;

  // Stop when found or at the end
  while ( timerSrch )
  {
    if ( timerSrch->eventFlag == event_flag &&
         timerSrch->id == task_id )
    {
      break;
    }

    // Not this one, check another
    timerSrch = timerSrch->next;
  }

  return ( timerSrch );
}





int startTimerEx( timerList_t timerList,int taskID, int event_id, int timeout_value )
{
	int res;
  // Add timer
	res = addTimer( timerList,taskID, event_id, timeout_value );
	return res;
}



int stopTimerEx( timerList_t timerList,int task_id, int event_id )
{
	timerRec_t timerSrch;
  // Find the timer to stop
	
	timerSrch = findTimer( timerList,task_id, event_id );
	if ( timerSrch ){
		timerSrch->eventFlag = 0;
		timerSrch->timeout =0;
		if(timerSrch->prev !=NULL)//if head
			timerSrch->prev->next = timerSrch->next;
		else timerList->head = timerSrch->next;
		
		if(timerSrch->next!=NULL)//if tail
			timerSrch->next->prev = timerSrch->prev;
		else timerList->tail = timerSrch->prev;
		
		timerSrch->next = timerList->cache;
		timerSrch->prev = NULL;
		timerList->cache = timerSrch; //into cache
		timerList->cachesize++;
		timerList->activesize--;
		
		return 0;
	}
	
	return 1;
}



int getTimeoutEx(timerList_t timerList, int task_id, int event_id )
{
  int rtrn = 0;
  timerRec_t tmr;

  tmr = findTimer(timerList, task_id, event_id );

  if ( tmr )
  {
    rtrn = tmr->timeout;
  }

  return rtrn;
}


int getTimerActive( timerList_t timerList )
{
  return timerList->activesize;
}

int getTimerCache( timerList_t timerList )
{
  return timerList->cachesize;
}

int timerUpdate( timerList_t timerList,taskList_t taskList )
{
 
	timerRec_t timerPrev = NULL;
	timerRec_t timerSrch = NULL;
	getCurentTime();
	// Look for open timer slot
	if ( timerList->head != NULL )
		timerSrch = timerList->head;
	else return 1;	
	
    // Look for open timer slot
    while (timerSrch)
    {
		if ((timerSrch->timeout <= currentTime)  && (timerSrch->eventFlag) ){
			setTaskEvent( taskList,timerSrch->id, timerSrch->eventFlag );
			timerPrev = timerSrch->next;
			timerSrch->eventFlag = 0;
			
			//repair the list
			if(timerSrch->prev !=NULL)//if head
				timerSrch->prev->next = timerSrch->next;
			else timerList->head = timerSrch->next;
			
			if(timerSrch->next!=NULL)//if tail
				timerSrch->next->prev = timerSrch->prev;
			else timerList->tail = timerSrch->prev;
			
			timerSrch->next = timerList->cache;
			timerSrch->prev = NULL;
			timerList->cache = timerSrch; //into cache
			timerList->cachesize++;
			timerList->activesize--;
			timerSrch = timerPrev;  
		}
		else timerSrch = timerSrch->next;	
	} 
	return 0;
}

