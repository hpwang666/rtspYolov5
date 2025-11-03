#include "core.h"
#include "media.h" 
#include "zlog.h"

#include "connection.h"

#undef LOG_HANDLE
#define LOG_HANDLE
#ifdef  LOG_HANDLE
	#define log_info(...) {zlog_info(zc,__VA_ARGS__);printf(__VA_ARGS__);printf("\r\n");}
	#define log_err(...)  {zlog_error(zc,__VA_ARGS__);printf(__VA_ARGS__);printf("\r\n");}
#else
	#define log_info(...) printf(__VA_ARGS__)
	#define log_err(...) printf(__VA_ARGS__)
#endif

extern zlog_category_t *zc;
extern poolList_t list;
rtspClient_t rc[16];

//返回 1 都是需要更新的
int set_cam_chn(int chn,netConfig_t netCfg)
{
	if(rc[chn]) {
		log_info("%s:%s",rc[chn]->conn->peer_ip,netCfg->mediaInfo.camAddress);
		if(strcmp(rc[chn]->conn->peer_ip,netCfg->mediaInfo.camAddress)==0 && \
			strncmp((char *)rc[chn]->sess->url->data,netCfg->mediaInfo.camUrl,strlen(netCfg->mediaInfo.camUrl))==0){
			log_info("ip url no change");
			return 0;
		}else{
			free_rtsp_clients(rc[chn]);
			log_info("free rtsp client [%d]",chn);
			rc[chn] = NULL;
		}	
	}
	if(strlen(netCfg->mediaInfo.camAddress) == 0) return 1;
	if(strlen(netCfg->mediaInfo.camUrl) == 0) return 1;

	rc[chn] = init_rtsp_clients(list,netCfg->mediaInfo.camAddress,netCfg->mediaInfo.camPort,\
				   netCfg->mediaInfo.camUser,netCfg->mediaInfo.camPasswd,netCfg->mediaInfo.camUrl);
	rc[chn]->chn = chn;
	return 1;
}


//从多画面  切换到少画面时，会有遗留的rc继续接收流
//需要将其关掉

void free_old_subwin(int current,int old)
{
	int i;
	if(old == current) return;
	for(i=0;i<old;i++){
		if(rc[i])  free_rtsp_clients(rc[i]);
		rc[i] = NULL;
	}
}


void init_media()
{
	int i;
	for(i=0;i<16;i++) rc[i]=NULL;
	return;
}

void free_media(void)
{
	int i =0;
	for(i=0;i<16;i++){
		if(rc[i]) free_rtsp_clients(rc[i]);
		rc[i] =NULL;
	}
}
