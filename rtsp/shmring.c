
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "shmring.h"
extern rtpPkg_t rtpPkg;
extern buf_t unixBuf;
void send_pkg(conn_t c,int id,u_char* data,int size)
{
	sprintf(rtpPkg->magic,"nihaoya");
	rtpPkg->len=size;
	rtpPkg->id=id;
	memcpy(rtpPkg->data,data,size);
#if 0
	c->send(c,(u_char *)rtpPkg,sizeof(struct rtpPkg_st));
#else
	buf_extend(unixBuf,sizeof(struct rtpPkg_st));
	memcpy(unixBuf->tail,(u_char *)rtpPkg,sizeof(struct rtpPkg_st));
	unixBuf->size+=sizeof(struct rtpPkg_st);
	unixBuf->tail+=sizeof(struct rtpPkg_st);
	add_event(c->write, WRITE_EVENT);
#endif
}
	
	
