
#ifndef _ALLTASKS_H
#define _ALLTASKS_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define OSAL_PRINT1 (0x00000002) 




#define OSAL_CHECK (0x08000000) 


int osal_printf(int task_id,int events);
int dec_reset_handler(int task_id,int events);
int check_left_freams(int task_id,int events);


#endif