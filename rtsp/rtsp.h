#ifndef _RTSP_H
#define _RTSP_H

#include "core.h"
#include "conutil.h"

#define RTSPERR -1
#define RTSP200 (200)
#define RTSP401 (401)
#define RTSP301 (301)
#define RTSP302 (302)
typedef struct rtspClient_st  	*rtspClient_t; 
typedef struct mediaSession_st  *mediaSession_t; 
typedef int (*rtsp_handler)(rtspClient_t rc);
 
typedef enum{RTSP_BAD=-1,RTSP_MIN=0,RTSP_TXT=1,RTSP_BIN,RTSP_NO_H264} RTSP_TYPE;

struct rtspClient_st{
	conn_t conn;
	pool_t pool;
	
	str_t	user;
	str_t	passwd;
	rtsp_handler do_next;
	mediaSession_t sess;
	int cseq;
	int chn;
	unsigned success:1; //成功获取码流
};

struct mediaSession_st{
	str_t url;//新增内容只能加下面，否则会影响set zero
	str_t play;
	str_t session;
	
	str_t realm;
	str_t nonce;
	str_t transport;
	str_t auth;
	str_t control;
	str_t contentBase;
	str_t vps;
	str_t sps;
	str_t pps;
};

rtspClient_t init_rtsp_clients(poolList_t list,char *ip,int port,char *user,char *passwd,char *url);
int free_rtsp_clients(rtspClient_t rc);

#endif
