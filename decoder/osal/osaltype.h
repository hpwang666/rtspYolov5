#ifndef _OSALTYPE_H
#define _OSALTYPE_H



#ifndef NULL
#define NULL (0)
#endif

//all event flags,
#define OSALSTART 	(0x80000000)
#define OSALTIMER 	(0x40000000)
#define OSALMSG 	(0x20000000)
#define OSALERR     (0x10000000)
#define OSALSTOP 	(0x00000000) 


//the rest bits is used for user's define

//lcd

#define flag_base 0X00000001

typedef struct osalEnv_st  * osalEnv_t;
typedef struct timerRec_st *timerRec_t;
typedef struct timerList_st  *timerList_t;
typedef struct osalTask_st *osalTask_t;
typedef struct taskList_st  *taskList_t;
typedef struct msg_st *msg_t;
typedef struct msgBox_st  *msgBox_t;


struct osalEnv_st {
    timerList_t  timerList;
	taskList_t  taskList;
    msgBox_t	msgBox;
	pthread_mutex_t lock;
	char err[32];//record the latest err
} ;


struct timerRec_st
{
  int   id; 
  int 	eventFlag;
  unsigned long long   timeout; //ms  
  timerRec_t next;
  timerRec_t prev;
} ;

struct timerList_st {
    timerRec_t  cache;

    timerRec_t  head;
    timerRec_t  tail;

    int         activesize;
    int			cachesize;
} ;


typedef int (*pTaskEventHandlerFn)(int id,int event );

struct osalTask_st
{
  int   id; 
  int 	eventFlag;
  pTaskEventHandlerFn handle;
  osalTask_t next;
} ;


struct taskList_st {
    osalTask_t  head;
    int         taskNum;
} ;



struct msg_st
{
  int   id; 
  char  msg[64];
  msg_t next;
  msg_t prev;
} ;



struct msgBox_st {
    msg_t  head;
	msg_t  tail;
	msg_t  cache;
    int             activesize;
    int				cachesize;
} ;

#endif
