#ifndef _MEDIA_H
#define _MEDIA_H

#include "rtsp.h"


typedef struct netConfig_st  *netConfig_t;

#define PKG_MAGIC  (0x123456)
#define CFG_SUBWIN (88)
#define CFG_CHNERR  (89)

struct mediaInfo_st{
	
	int 	camPort;
	int		res;
	char    camAddress[64];
	char    camUrl[128];
	char	camUser[32];  
	char   	camPasswd[32];
	
};

struct netConfig_st{
	int     magic;
	int chn;
	int subwin; //1 4 6 9 16
	struct mediaInfo_st mediaInfo;//
};

struct chnStatus_st{ //和解码器通信的接口
	int id;
	int err;
	int update; //表示该通道切换了视频源，可以进行reset操作
	int subwin;
};

int find_cam(char *ip,char *url);
int set_cam_chn(int chn,netConfig_t);
void modify_cam_chn(int oldchn,int newchn);
void free_old_subwin(int current,int old);
void init_media(void);
void free_media(void);

#endif
