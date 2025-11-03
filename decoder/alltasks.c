
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "alltasks.h"
#include "osal.h"
#include "decoder.h"



int osal_printf(int task_id,int events)
{
	int do_event = events;
	//VDEC_CHN_STAT_S pstStat;
	if( events & OSALSTART){
		printf("****osal start\n");
		osalStartTimerEx(task_id,OSAL_PRINT1,1000);
		do_event ^= OSALSTART;
	}
	if ( events & OSAL_PRINT1 )
	{
		//FY_MPI_VDEC_Query(0,&pstStat);
		// printf("%d %d %d %d %d %d %d %x\n",pstStat.stVdecDecErr.s32FormatErr,pstStat.stVdecDecErr.s32PicSizeErrSet,\
		// 	pstStat.stVdecDecErr.s32StreamUnsprt,pstStat.stVdecDecErr.s32PackErr,pstStat.stVdecDecErr.s32PrtclNumErrSet,
		// 	pstStat.stVdecDecErr.s32RefErrSet,pstStat.stVdecDecErr.s32PicBufSizeErrSet,pstStat.stVdecDecErr.s32VdecStreamNotRelease);

		// printf("RecvFrames:%d , decdFrames:%d\n",pstStat.u32RecvStreamFrames,pstStat.u32DecodeStreamFrames);
		// printf("leftBytes:%d, LeftFrames:%d %d \n\n",pstStat.u32LeftStreamBytes,pstStat.u32LeftStreamFrames,pstStat.u32LeftPics);
		osalStartTimerEx(task_id,OSAL_PRINT1,150000);
		do_event ^= OSAL_PRINT1;
	}
	return do_event;
}


int dec_reset_handler(int task_id,int events)
{
	int do_event = events;
	int ret=0;
	int i ;
	
	for(i=0;i<16;i++){
		if(events & (0x00000001<<i)){
#if 0
			//清除屏幕上的缓存数据
			ret= FY_MPI_VO_ClearChnBuffer(FY_VO_LAYER_VHD0 , i,FY_TRUE);
			if (FY_SUCCESS != ret)
			{
				printf("pause vo chn failed! \n");
			}
			else{
				printf("clear chn %d \r\n",i);
			}
			FY_MPI_VO_FillChn(0,i,0x0);
			FY_MPI_VO_RefreshChn(FY_VO_LAYER_VHD0 , i);
#endif
			//osalStartTimerEx(task_id,(0x00000001<<i),500);
			do_event ^=(0x00000001<<i);
		}
	}	
	return do_event;
}

int check_left_freams(int task_id,int events)
{
	int do_event = events;
	int i=0;
	//VDEC_CHN_STAT_S pstStat;
	if( events & OSALSTART){
		printf("***osal check start***\n");
		osalStartTimerEx(task_id,OSAL_CHECK,1000);
		do_event ^= OSALSTART;
	}

	if ( events & OSAL_CHECK )
	{
		for(i=0;i<16;i++){
			//FY_MPI_VDEC_Query(i,&pstStat);
			
			//if(0) printf("decdFrames:%d, LeftFrames:%d  \n\n",pstStat.u32DecodeStreamFrames,pstStat.u32LeftStreamFrames);
			// if(pstStat.u32LeftStreamFrames>50){
			// 	printf("chn: %d stop recving  for too much leftbytes !\n",i); 
			// 	if(decEnv->decs[i].err){ 
			// 		break; 
			// 	} 
			// 	dec_buf_init(decEnv->decs[i].buf); 
			// 	decEnv->decs[i].refused=1;
			// 	osalStartTimerEx(task_id,0x00000001<<i,2200);
			// }
		} 
		osalStartTimerEx(task_id,OSAL_CHECK,1000);
		do_event ^= OSAL_CHECK;
	}

	for(i=0;i<16;i++){
		if(events & (0x00000001<<i)){
			printf("CHN %d STARTING RECV FREAMS !\n",i); 
			decEnv->decs[i].refused=0;
			do_event ^=(0x00000001<<i);
		}
	}	

	return do_event;
}

