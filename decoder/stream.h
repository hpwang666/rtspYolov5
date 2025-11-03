#ifndef _STREAM_H
#define _STREAM_H


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include "buf.h"
#include "decoder.h"

int process_rtp(u_char *bufIN, size_t inLen,int chn, decoder_t,int);
int UnpackRTPH264( u_char *bufIN, size_t len, decoder_t);
int UnpackRTPH265( u_char *bufIN, size_t len, decoder_t);
int make_seqc_right(u_char *bufIN, size_t len, int chn ,decoder_t dec);
int do_decode(decoder_t dec,int chn,int rtpType);
int parase_pps(u_char *buf,int len,HEVCPPS_t hevcPPS);
#define RTP_PPS 		(0x00000001)
#define RTP_SPS 		(0x00000002)
#define RTP_I			(0x00000004)
#define RTP_P			(0x00000008)
#define RTP_PKG			(0x00000020)
#define RTP_VPS			(0x00000040)

#define RTP_ERR         (0x00000010)
#endif
