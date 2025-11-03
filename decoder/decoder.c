#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <time.h>
#include <sys/socket.h>
#include <fcntl.h>  

#include "decoder.h"
#include "nvdec.h"


using namespace std;

decEnv_t create_dec_chns(void)
{
	int i=0;
	decEnv_t decEnv = (decEnv_t)calloc(1,sizeof(struct decEnv_st));
	decEnv->decs= (decoder_t)calloc(CHNS,sizeof(struct decoder_st));



	for(i=0;i<CHNS;i++)
	{
		
		decEnv->decs[i].buf = dec_buf_new(1024*1024);
		decEnv->decs[i].PKG_STARTED = 0;


		decEnv->decs[i].hevcPPS=(HEVCPPS_t)calloc(1,sizeof(struct HEVCPPS_st));

		
		
		if(i==0)
			set_defaults(&(decEnv->decs[i].ctx),0,0,960,540);
		if(i==1)
			set_defaults(&(decEnv->decs[i].ctx),960,0,960,540);

		if(i==2)
			set_defaults(&(decEnv->decs[i].ctx),0,540,640,360);
		if(i==3)
			set_defaults(&(decEnv->decs[i].ctx),960,540,640,360);

		sprintf(decEnv->decs[i].ctx.decName,"dec%d",i);
		sprintf(decEnv->decs[i].ctx.DecCapPlane,"DecCapPlane%d",i);
		sprintf(decEnv->decs[i].ctx.RendererName,"renderer%d",i);

		initDecoder(decEnv->decs[i].ctx);
		
	}
	return decEnv;
}

void free_dec_chns(decEnv_t decEnv)
{
	int i;
	for(i=0;i<CHNS;i++){
		freeDecoder(decEnv->decs[i].ctx);
		dec_buf_free(decEnv->decs[i].buf);
		free(decEnv->decs[i].hevcPPS);
	}
}
