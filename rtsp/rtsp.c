/*added in 2020/4/13*/


#include "core.h"
#include "connection.h"
#include "timer.h"
#include "event.h"
#include "rtsp.h"
#include "md5.h"
#include "shmring.h"
#include "media.h"
#include "base64.h"
#include "zlog.h"


#undef LOG_HANDLE
//#define LOG_HANDLE
#ifdef  LOG_HANDLE
	#define log_debug(...) 
    #define log_info(...) {zlog_info(zc,__VA_ARGS__);printf(__VA_ARGS__);printf("\r\n");}
	#define log_err(...)  {zlog_error(zc,__VA_ARGS__);printf(__VA_ARGS__);printf("\r\n");}
#else
	#define log_debug(...) 
	#define log_info(...) 
	#define log_err(...) 
#endif

extern zlog_category_t *zc;
struct chnStatus_st chnStatus;
extern conn_t unixFd;

static int rtsp_connect_handler(event_t ev);
static int rtsp_reconnect_peer(event_t ev);
static int rtsp_read_handle(event_t ev);
static int rtsp_rewrite_handle(event_t ev);

static int do_response(rtspClient_t rc,u_char* data, size_t len);
static RTSP_TYPE get_resp_len(buf_t buf, size_t *PkgLen);
static int  search_rtsp_head(buf_t buf);
static void generate_auth(rtspClient_t rc, char* cmd);
static int send_option(rtspClient_t rc);
static int send_describe(rtspClient_t rc);
static int send_setup(rtspClient_t rc);
static int send_play(rtspClient_t rc);
static int send_teardown(rtspClient_t rc);


rtspClient_t init_rtsp_clients(poolList_t list,char *ip,int port,char *user,char *passwd,char *url)
{
	int ret;
	
	conn_t c;
	pool_t pool = get_pool(list,DEFAULT_POOL_SIZE);
	rtspClient_t rtspClient = (rtspClient_t)palloc(pool,sizeof(struct rtspClient_st));
	rtspClient->pool=pool;
	rtspClient->sess = (mediaSession_t)palloc(pool,sizeof(struct mediaSession_st));
	rtspClient->success = 0;
	str_t_dup(pool,rtspClient->user,user);
	str_t_dup(pool,rtspClient->passwd,passwd);
	
	str_t_dup(pool,rtspClient->sess->url,url);
	
	ret = connect_peer(ip,port,&(rtspClient->conn));
	c = rtspClient->conn;
	c->data = rtspClient;
	if(ret == AIO_AGAIN){
		c->read->handler = rtsp_connect_handler;
		c->write->handler = rtsp_connect_handler;//write超时函数
		add_timer(c->write, 2000);//将时间改长了一点
	}
	if(ret == AIO_OK);
	return rtspClient;
}

int free_rtsp_clients(rtspClient_t rc)
{
	close_conn(rc->conn);
	if(rc->pool) destroy_pool(rc->pool);
	rc->pool=NULL;
	return 0;
}

static int rtsp_connect_handler(event_t ev)
{
    conn_t      c;
    //ngx_stream_session_t  *s;

    c = ev->data;
    rtspClient_t rc = c->data;

    if (ev->timedout) {
        log_info("CONN:conn timeout");
		chnStatus.id = rc->chn;
		chnStatus.err =1;
		send_pkg(unixFd,CFG_CHNERR,(u_char *)&chnStatus,sizeof(struct chnStatus_st));
		ev->timedout = 0;
		if(AIO_AGAIN == rtsp_reconnect_peer(ev)){
			return AIO_AGAIN;
		}
    }
	
    if (test_connect(c) != AIO_OK) {//handled  by wev->handler
        log_info("TEST_CONN:conn failed");
		chnStatus.id = rc->chn;
		chnStatus.err =1;
		send_pkg(unixFd,CFG_CHNERR,(u_char *)&chnStatus,sizeof(struct chnStatus_st));
        return AIO_ERR;
    }
	
	del_timer(c->write);
	set_conn_info(c);
	log_info("Connected,client=%s:%d peer=%s:%d", c->local_ip,c->local_port,c->peer_ip,c->peer_port);
	
	c->write->ready = 1;//可写
	add_event(c->read,READ_EVENT);
	c->read->handler =rtsp_read_handle;
	c->write->handler = rtsp_rewrite_handle;
   
	//add_timer(c->write, 50000);//定时 tear down
	//add_timer(c->read, 800);
	send_option(c->data);
	return 0;
}


static int rtsp_reconnect_peer(event_t ev)
{
	struct sockaddr_in sa;
	int rc;
	conn_t      c;
	c = ev->data;
	
	if (c->read->timer_set)  del_timer(c->read); 
    if (c->write->timer_set) del_timer(c->write);       
	if (c->read->active) 	del_event(c->read,0,CLOSE_EVENT);	
	if (c->write->active ) 	del_event(c->write,0,CLOSE_EVENT);
	if(-1 != c->fd)	 {close(c->fd); c->fd =-1;} //注意，close fd 之后，所有的epoll  event 都失效了
	buf_init(c->readBuf);
	
	sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(c->peer_ip);	
    sa.sin_port = htons(c->peer_port);			
	
    if((c->fd = socket(AF_INET,SOCK_STREAM,0)) < 0) return -1;

	nonblocking(c->fd);
	keepalive(c->fd);
	
    rc = connect(c->fd,(struct sockaddr*)&sa, sizeof(struct sockaddr_in));
	
    if(rc == 0)
    {
        log_info("RECONN:already ok?");
		add_event(c->read, READ_EVENT);
		return AIO_OK;
    }
	
	if(rc == -1 && (CONN_WOULDBLOCK || CONN_INPROGRESS))//非阻塞都会执行到这一步
    {
        log_info("RECONN:need check");
		c->write->handler = rtsp_connect_handler;
		add_event(c->write, WRITE_EVENT);
		add_timer(c->write, 5000);
		return AIO_AGAIN;
    }
	
	free_rtsp_clients(c->data);
	return AIO_ERR;
}


static int rtsp_read_handle(event_t ev)
{
	int r;
	int rep;
	RTSP_TYPE rtspType;
	size_t rtpPkgLen;
	u_char * cc;
	conn_t c = (conn_t)ev->data;
	rtspClient_t rc = c->data;
	buf_t buf=c->readBuf;
	pool_t p;
	
	if (ev->timedout) {//数据读取超时
		//close_conn(c);//此处不能close，会释放conn，引起错误指针
		log_info("read timeout");
		ev->timedout = 0;
		rc->success = 0;
		cc=(u_char *)(rc->sess->url)+sizeof(struct str_st)+rc->sess->url->size+1;
		//printf("%c,%c\n",*(cc-2),*(cc-3));//显示url最后两个字母
		rc->pool->last = cc;
		memset (cc,0,rc->pool->end - cc +1);
		
		p=rc->pool;	
		p->current = p;
		while(p->next){
			printf("reset one pool\r\n");
			p=p->next;
			p->last = (u_char *) p + sizeof(struct pool_st);
			p->failed = 0;
			memset(p->last,0,(p->end-p->last+1));
		}	
		rc->sess->play=NULL;
		rc->sess->realm=NULL;
		rc->sess->session=NULL;
		rc->sess->nonce=NULL;
		rc->sess->transport=NULL;
		rc->sess->auth=NULL;
		rc->sess->control = NULL;
		rc->sess->contentBase=NULL;
		rc->sess->pps=NULL;
		rc->sess->sps=NULL;
		c->write->handler = rtsp_connect_handler;
		add_timer(c->write, 0);
		chnStatus.id = rc->chn;
		chnStatus.err =1;
		send_pkg(unixFd,CFG_CHNERR,(u_char *)&chnStatus,sizeof(struct chnStatus_st));
		return AIO_ERR;
    } 
	if(rc->success)//避免密码错误发生反复请求
		add_timer(c->read, 4000);
	
	buf_extend(buf, 4096);
	r = c->recv(c,buf->tail,4096);
	if(r<=0){
		if(r==0){
			//只要不是主动teardown的，就都要重连
			//free_rtsp_clients(c->data);
			log_info("peer conn closed:%s:%d\n",c->peer_ip,c->peer_port);
			rc->success = 0;
			cc=(u_char *)(rc->sess->url)+sizeof(struct str_st)+rc->sess->url->size+1;
			//printf("%c,%c\n",*(cc-2),*(cc-3));//显示url最后两个字母
			rc->pool->last = cc;
			memset (cc,0,rc->pool->end - cc +1);

			p=rc->pool;
			p->current = p;
			while(p->next){
				printf("reset one pool\r\n");
				p=p->next;
				p->last = (u_char *) p + sizeof(struct pool_st);
				p->failed = 0;
				memset(p->last,0,(p->end-p->last+1));
			}	
			rc->sess->play=NULL;
			rc->sess->realm=NULL;
			rc->sess->session=NULL;
			rc->sess->nonce=NULL;
			rc->sess->transport=NULL;
			rc->sess->auth=NULL;
			rc->sess->control = NULL;
			rc->sess->contentBase=NULL;
			rc->sess->pps=NULL;
			rc->sess->sps=NULL;
			c->write->handler = rtsp_connect_handler;
			add_timer(c->write, 5000);
		}
		else handle_read_event(ev);
	}
	else{
		buf->size += r;
		buf->tail += r;
		rtspType = get_resp_len(buf, &rtpPkgLen);
		while(rtspType >0 )
		{
			if(rtspType == RTSP_BIN){
				send_pkg(unixFd, rc->chn,buf->head,rtpPkgLen);
			}
			
			if(rtspType == RTSP_TXT){
				log_debug("<<<\n");
				for(rep=0;rep<rtpPkgLen;rep++)
					log_debug("%c",*(buf->head+rep));
			
				rep = do_response(rc,buf->head,buf->size);
				switch (rep){
					case RTSP200:
						if(rc->do_next)
							rc->do_next(rc);
						break;
					case RTSP401:
						if(!rc->sess->auth){
							generate_auth(rc,"DESCRIBE");
							send_describe(rc);
						}else {
							printf("wrong user or passwd \n");
							chnStatus.id = rc->chn;
							chnStatus.err =1;
							send_pkg(unixFd,CFG_CHNERR,(u_char *)&chnStatus,sizeof(struct chnStatus_st));
						}
						break;
					case RTSPERR:
						printf("err in do_response\n");
					default:break;
				}
			}
					
			buf_consume(buf, rtpPkgLen);	
			rtspType = get_resp_len(buf, &rtpPkgLen);
		}	
		if( rtspType == RTSP_BAD){
			printf("RTSP>>>>>>>bad package\r\n");
			int ret = search_rtsp_head(buf);
			buf_consume(buf, ret);	
		}
		if( rtspType == RTSP_MIN){
			//printf("RTSP>>>>>>>min package\r\n");
		}
		
		if(ev->ready){
			handle_read_event(ev);
		}
	}
	return AIO_OK;
}



static int rtsp_rewrite_handle(event_t ev)
{
	int n;
	conn_t c = ev->data;
	if (ev->timedout) {
        printf("tear_down....\r\n");
		ev->timedout = 0;
		send_teardown(c->data);
    }
	
	n = c->send(c,(u_char *)"send again",10);
	if(!c->write->ready) printf("only send %d bytes\n",n);
	return 0;
}


static int send_option(rtspClient_t rc)
{
	str_t opt;
	conn_t c=rc->conn;
	str_t_ndup(rc->pool,opt,128);
	str_t_append(opt,"OPTIONS rtsp://",15);
	str_t_append(opt,c->peer_ip,strlen(c->peer_ip));

	str_t_cat(opt,rc->sess->url);

	str_t_append(opt," RTSP/1.0\r\n",11);
	
	rc->cseq=1;
	str_t_sprintf(opt,"CSeq: %d\r\n",rc->cseq);
	
	
	str_t_append(opt,"\r\n\r\n",4);
	rc->do_next = send_describe;
	
	log_debug(">>opt\n");
	log_debug("%s",opt->data);
	
	c->send(c,opt->data,opt->len);
	if(c->write->ready) {
		
		//printf("write ok \n");
	}
	else add_event(c->write,WRITE_EVENT);
	
	return 0;
}


//<  DESCRIBE rtsp://192.168.1.26/h264/ch1/main/av_stream RTSP/1.0
// <  CSeq: 2
// <  Accept: application/sdp

static int send_describe(rtspClient_t rc)
{
	str_t opt;
	conn_t c=rc->conn;
	str_t_ndup(rc->pool,opt,640);
	str_t_append(opt,"DESCRIBE rtsp://",16);
	str_t_append(opt,c->peer_ip,strlen(c->peer_ip));

	str_t_cat(opt,rc->sess->url);

	str_t_append(opt," RTSP/1.0\r\n",11);
	
	rc->cseq++;
	str_t_sprintf(opt,"CSeq: %d\r\n",rc->cseq);
	
	
	if(rc->sess->auth){
		str_t_append(opt,"Authorization: ",15);
		str_t_cat(opt,rc->sess->auth);
		str_t_append(opt,"\r\n",2);
	}
	str_t_append(opt,"Accept: application/sdp",23);
	str_t_append(opt,"\r\n\r\n",4);
	
	rc->do_next = send_setup;
	
	log_debug(">>>>desr\n");
	log_debug("%s",opt->data);
	c->send(c,opt->data,opt->len);
	if(c->write->ready) {
		//printf("write ok \n");
	}
	else add_event(c->write,WRITE_EVENT);
	
	return 0;
}

// <  SETUP rtsp://192.168.1.26/h264/ch1/main/av_stream/trackID=1 RTSP/1.0
// <  CSeq: 4
// <  Authorization: Digest username="admin", realm="4cbd8fd0f264", nonce="84163eff80cfd69b2b4a54778870b877", uri="rtsp://192.168.1.26/h264/ch1/main/av_stream/trackID=1", response="2e03cb6ab26dd5d30692620b2767a5dc"
// <  Transport: RTP/AVP/TCP;unicast;interleaved=0-1
static int send_setup(rtspClient_t rc)
{
	str_t opt;
	conn_t c=rc->conn;
	str_t_ndup(rc->pool,opt,512);
	generate_auth(rc,"SETUP");
	str_t_append(opt,"SETUP ",6);
	//printf("%d:%s  %d:%s\n",rc->sess->control->len,rc->sess->control->data,rc->sess->contentBase->len,rc->sess->contentBase->data);
	if(rc->sess->control){
		if(rc->sess->contentBase){
			if(str_nstr((u_char *)rc->sess->control->data,(char *)rc->sess->contentBase->data,rc->sess->control->len))
			{
				log_debug(">>>found base url in control\n");
				str_t_cat(opt,rc->sess->control);
			}
			else {
				str_t_cat(opt,rc->sess->contentBase);
				str_t_cat(opt,rc->sess->control);
			}
		}
		else str_t_cat(opt,rc->sess->control);
	}
	else printf("no control\r\n");
	str_t_append(opt," RTSP/1.0\r\n",11);
	
	rc->cseq++;
	str_t_sprintf(opt,"CSeq: %d\r\n",rc->cseq);
	
	str_t_append(opt,"Transport: RTP/AVP/TCP;unicast;interleaved=0-1",46);
	str_t_append(opt,"\r\n",2);
	str_t_append(opt,"Authorization: ",15);
	str_t_cat(opt,rc->sess->auth);
	//str_t_append(opt,"\r\n",2);
	str_t_append(opt,"\r\n\r\n",4);
	
	rc->do_next = send_play;
	
	log_debug(">>>>setup\n");
	log_debug("%s",opt->data);
	c->send(c,opt->data,opt->len);
	if(c->write->ready) {
		//printf("write ok \n");
	}
	else add_event(c->write,WRITE_EVENT);
	return 0;
}

static int send_play(rtspClient_t rc)
{
	str_t opt;
	conn_t c=rc->conn;
	str_t_ndup(rc->pool,opt,512);
	str_t_append(opt,"PLAY ",5);
	str_t_cat(opt,rc->sess->play);

	str_t_append(opt," RTSP/1.0\r\n",11);
	
	rc->cseq++;
	str_t_sprintf(opt,"CSeq: %d\r\n",rc->cseq);
	
	str_t_append(opt,"Session: ",9);
	str_t_cat(opt,rc->sess->session);
	str_t_append(opt,"\r\n",2);
	str_t_append(opt,"Range: npt=0.000-\r\n",19);
	str_t_append(opt,"Authorization: ",15);
	str_t_cat(opt,rc->sess->auth);
	str_t_append(opt,"\r\n\r\n",4);
	
	rc->do_next =NULL;
	rc->success = 1;
	log_debug(">>>>play\n");
	log_debug("%s",opt->data);
	c->send(c,opt->data,opt->len);
	if(c->write->ready) {
		//printf("write ok \n");
	}
	else add_event(c->write,WRITE_EVENT);
	
	
	
	return 0;
}

static int send_teardown(rtspClient_t rc)
{
	str_t opt;
	conn_t c=rc->conn;
	str_t_ndup(rc->pool,opt,256);
	str_t_sprintf(opt,"TEARDOWN rtsp://%s",c->peer_ip);
	

	str_t_cat(opt,rc->sess->url);
	str_t_sprintf(opt," RTSP/1.0\r\n");
	
	rc->cseq++;
	str_t_sprintf(opt,"CSeq: %d\r\n",rc->cseq);
	
	str_t_sprintf(opt,"Session: ");
	str_t_cat(opt,rc->sess->session);
	str_t_sprintf(opt,"\r\n\r\n");
	
	rc->do_next =free_rtsp_clients;
	
	//log_debug(">>>>\n");
	//log_debug("%s",opt->data);
	c->send(c,opt->data,opt->len);
	if(c->write->ready) {
		//printf("write ok \n");
	}
	else add_event(c->write,WRITE_EVENT);
	
	return 0;
}

static RTSP_TYPE get_resp_len(buf_t buf, size_t *PkgLen)
{
	size_t size = buf->size;
	u_char *data = buf->head;
	size_t headlen;
	u_char* q;
	*PkgLen =0;
	if(size < 4)
		return RTSP_MIN;
	if(data[0] == '$'){
		// this is an rtp packet.
		int len = (data[2] & 0xFF) << 8 | (data[3] & 0xFF);
		if(size >=(len + 4) ){
			*PkgLen = len + 4;
			//printf("bin type: %02x \r\n",data[4]&0x3f);
			//printf("bin type: %02x \r\n",data[5]&0x7f);
			if((data[5]&0x7f)!=96)
				return RTSP_NO_H264;//h265也是96，这个表示视频
			else
				return RTSP_BIN;
		}
		else {
			return RTSP_MIN;
		}
	} else{
		// rtsp response data.
		if(strncmp((char *)data, "RTSP", 4)!=0)
			return RTSP_BAD;
			
		u_char *p =str_nstr(data , "\r\n\r\n", size);
		if(!p)
			return RTSP_MIN;
		
		headlen = p - data;
		q = str_nstr(data, "\r\nContent-Length: ", headlen);
		if(!q)
			q = str_nstr(data, "\r\nContent-length: ", headlen);
		*PkgLen = headlen + 4 + (q ? atoi((char *)(q + 17)) : 0);
		return RTSP_TXT;
	}
}

static int  search_rtsp_head(buf_t buf)
{
	u_char *data = buf->head;
	size_t size = buf->size;
	size_t i =0;
	
	for(i=0;i<size-1;i++)
	{
		if((data[i] == 0x24) && (data[i+1] == 0x00))
		return i;
	}
	
	u_char *p = str_nstr(data, "RTSP", size);
	if (p) return (p-data); 
	
	return size;
}

static int do_response(rtspClient_t rc,u_char* data, size_t len)
{
	int rep;
	char *head,*tail;
	char ppsB64[128];
	char spsB64[128];
	char vpsB64[128];
	int decodeLen=0;
	head =(char *)str_nstr(data,"RTSP/1.0 ",len);
	if(!head) return RTSPERR;
	rep = atoi(head+9);
	
	if(rep == RTSP401){
		head = (char *)str_nstr(data, "nonce",len);
		if(!head) 
		{
			head = (char *)str_nstr(data, "Basic",len);
			if(!head) return RTSPERR;
		}
		else{
			head+=7;
			tail = strchr(head,'"');
			if(!tail) return RTSPERR;
			str_t_ndup(rc->pool,rc->sess->nonce,64);
			str_t_append(rc->sess->nonce,head,tail-head);

			head =(char *) str_nstr(data,"realm",len);
			if(!head) return RTSPERR;
			head+=7; 
			tail = strchr(head,'"');
			if(!tail) return RTSPERR;
			str_t_ndup(rc->pool,rc->sess->realm,64);
			str_t_append(rc->sess->realm,head,tail-head);
		}
	}
	head = (char *)str_nstr((u_char *)data,"control:",len);
	if(head){
		tail=(char *)str_nstr((u_char *)head,"\r\n",len-((u_char*)head-data));
		if(tail){
			head+=8;
			str_t_ndup(rc->pool,rc->sess->play,128);
			str_t_append(rc->sess->play,head,tail-head);
		}
	}
	head = (char *)str_nstr((u_char *)data,"m=video",len);//获取视频的轨道
	if(head ){
		head+=7;
		head = (char *)str_nstr((u_char *)head,"control:",len-((u_char*)head-data));
		if(head){
			tail=(char *)str_nstr((u_char *)head,"\r\n",len-((u_char*)head-data));
			if(tail){
				head+=8;
				str_t_ndup(rc->pool,rc->sess->control,128);
				str_t_append(rc->sess->control,head,tail-head);
			}
		}
	}
	head = (char *)str_nstr((u_char *)data,"Content-Base:",len);
	if(head ){
		tail=(char *)str_nstr((u_char *)head,"\r\n",len-((u_char*)head-data));
		if(tail){
			head+=14;
			str_t_ndup(rc->pool,rc->sess->contentBase,128);
			str_t_append(rc->sess->contentBase,head,tail-head);
			}
	}

	memset(ppsB64,'\0',128);
	memset(spsB64,'\0',128);
	memset(vpsB64,'\0',128);
	head = (char *)str_nstr((u_char*)data,"sprop-parameter-sets",len);
	if(head){
		head +=strlen("sprop-parameter-sets")+1;
		tail = (char*)str_nstr((u_char *)head,",",64);
		if(tail){
			memcpy(spsB64,head,tail-head);
			str_t_ndup(rc->pool,rc->sess->sps,128);
			decodeLen=b64_decode_ex((u_char *)rc->sess->sps->data+16,spsB64,strlen(spsB64));
			rc->sess->sps->len =decodeLen+16; 
			head = tail+1;
			tail=(char *)str_nstr((u_char *)head,"\r\n",len-((u_char*)head-data));
			if(tail){
				memcpy(ppsB64,head,tail-head);
				str_t_ndup(rc->pool,rc->sess->pps,128);
				decodeLen=b64_decode_ex((u_char *)rc->sess->pps->data+16,ppsB64,strlen(ppsB64));
				rc->sess->pps->len = decodeLen+16;
				send_pkg(unixFd, rc->chn,rc->sess->sps->data,rc->sess->sps->len );
				send_pkg(unixFd, rc->chn,rc->sess->pps->data,rc->sess->pps->len );
				printf("get sps  pps \r\n");
			}
		}
	}

	//以下是针对H265获取 SPS PPS
	head = (char *)str_nstr((u_char*)data,"sprop-vps",len);
	if(head){
		head +=strlen("sprop-vps")+1;
		tail = (char*)str_nstr((u_char *)head,";",128);
		if(tail){
			memcpy( vpsB64,head,tail-head);
			str_t_ndup(rc->pool,rc->sess->vps,128);
			decodeLen=b64_decode_ex((u_char *)rc->sess->vps->data+16,vpsB64,strlen(vpsB64));
			rc->sess->vps->len =decodeLen+16; 
			send_pkg(unixFd, rc->chn,rc->sess->vps->data,rc->sess->vps->len );
			printf("get vps\r\n");
		}
	}
	head = (char *)str_nstr((u_char*)data,"sprop-sps",len);
	if(head){
		head +=strlen("sprop-sps")+1;
		tail = (char*)str_nstr((u_char *)head,";",128);
		if(tail){
			memcpy(spsB64,head,tail-head);
			str_t_ndup(rc->pool,rc->sess->sps,128);
			decodeLen=b64_decode_ex((u_char *)rc->sess->sps->data+16,spsB64,strlen(spsB64));
			rc->sess->sps->len =decodeLen+16; 
			head = tail+1;
			head = (char *)str_nstr((u_char*)tail,"sprop-pps",64);
			tail=NULL;
			if(head){
				head +=strlen("sprop-pps")+1;
				tail=(char *)str_nstr((u_char *)head,"\r\n",len-((u_char*)head-data));
			}
			if(tail){
				memcpy(ppsB64,head,tail-head);
				str_t_ndup(rc->pool,rc->sess->pps,128);
				decodeLen=b64_decode_ex((u_char *)rc->sess->pps->data+16,ppsB64,strlen(ppsB64));
				rc->sess->pps->len = decodeLen+16;
				send_pkg(unixFd, rc->chn,rc->sess->sps->data,rc->sess->sps->len );
				send_pkg(unixFd, rc->chn,rc->sess->pps->data,rc->sess->pps->len );
				printf("get sps  pps \r\n");
			}
		}
	}
	head =(char *)str_nstr(data,"Session:",len);
	if(!head || rc->sess->session) return rep;//已经有了session，返回
	head+=8; 
	while(*head == ' ')
		head++;
	tail = strchr(head,';');
	if(!tail) return RTSPERR;

	str_t_ndup(rc->pool,rc->sess->session,64);
	str_t_append(rc->sess->session,head,tail-head);
	return rep;
}


static void generate_auth(rtspClient_t rc, char* cmd)
{
	str_t ha1Data;
	str_t base64Out,base64In;
	md5_t md5_a,md5_b,md5_c;
	conn_t c = rc->conn;
	if(rc->sess->nonce){
		// The "response" field is computed as:
		//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
		// or, if "fPasswordIsMD5" is True:
		//    md5(<password>:<nonce>:md5(<cmd>:<url>))

		md5_a = (md5_t) palloc(rc->pool,sizeof(struct md5_st));
		md5_b = (md5_t) palloc(rc->pool,sizeof(struct md5_st));
		md5_c = (md5_t) palloc(rc->pool,sizeof(struct md5_st));

		str_t_ndup(rc->pool,ha1Data,256);

		//md5_a(<username>:<realm>:<password>)
		str_t_cat(ha1Data,rc->user);
		str_t_append(ha1Data,":",1);
		str_t_cat(ha1Data,rc->sess->realm);
		str_t_append(ha1Data,":",1);
		str_t_cat(ha1Data,rc->passwd);
		md5_update(md5_a,ha1Data->data, ha1Data->len);
		md5_final(md5_a);

		//md5_b(<cmd>:<url>)
		str_t_zero(ha1Data);
		str_t_append(ha1Data,cmd,strlen(cmd));
		str_t_append(ha1Data,":",1);
		if(rc->sess->control){
			str_t_cat(ha1Data,rc->sess->control);
		}
		else{
			str_t_append(ha1Data,"rtsp://",7);
			str_t_append(ha1Data,c->peer_ip,strlen(c->peer_ip));
			str_t_cat(ha1Data,rc->sess->url);
		}
		md5_update(md5_b,ha1Data->data, ha1Data->len);
		md5_final(md5_b);

		//md5_c(md5_a:nonce:md5_b))
		str_t_zero(ha1Data);
		str_t_append(ha1Data,md5_a->result,32);
		str_t_append(ha1Data,":",1);
		str_t_cat(ha1Data,rc->sess->nonce);
		str_t_append(ha1Data,":",1);
		str_t_append(ha1Data,md5_b->result,32);
		
		md5_update(md5_c,ha1Data->data, ha1Data->len);
		md5_final(md5_c);
		
		str_t_ndup(rc->pool,rc->sess->auth,512);
		if(rc->sess->control){
			str_t_sprintf(rc->sess->auth, "Digest username=\"%s\", realm=\"%s\", "
					"nonce=\"%s\", uri=\"%s\", response=\"%s\"", rc->user->data,
					rc->sess->realm->data, rc->sess->nonce->data, rc->sess->control->data, md5_c->result);
			
		}
		else
			str_t_sprintf(rc->sess->auth, "Digest username=\"%s\", realm=\"%s\", "
					"nonce=\"%s\", uri=\"rtsp://%s%s\", response=\"%s\"", rc->user->data,
					rc->sess->realm->data, rc->sess->nonce->data, c->peer_ip,rc->sess->url->data, md5_c->result);
	} else {
		str_t_ndup(rc->pool,base64In,256);
		str_t_ndup(rc->pool, base64Out,400);
		str_t_ndup(rc->pool,rc->sess->auth,512);
		str_t_sprintf(base64In,"%s:%s",rc->user->data,rc->passwd->data);
		b64_encode(base64In->data, (char *)base64Out->data,base64In->len);
		str_t_sprintf(rc->sess->auth,"Basic %s",base64Out->data);
		log_debug("%s",(char*)rc->sess->auth);
	}
}
