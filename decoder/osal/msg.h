
#ifndef _MSG_H
#define _MSG_H

#include "osaltype.h"
#include "tasks.h"


msgBox_t msgBoxInit(void);
int pushMsg(msgBox_t msgBox,taskList_t taskList,int task_id, char *msg) ;
msg_t pullMsg(msgBox_t msgBox,int task_id) ;

#endif    /* INCL_UTIL_H */


