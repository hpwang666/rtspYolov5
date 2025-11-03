
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _SHMRING_H
#define _SHMRING_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "connet.h"
	
	
typedef struct rtpPkg_st  	*rtpPkg_t;




struct rtpPkg_st {
	char magic[8];
	int id ;
	int   len;
    u_char  data[1600];//初始化的时候按实际分配大小以及类型
};


void send_pkg(conn_t c,int id,u_char* data,int size);
#endif /* _PALLOC_H */
