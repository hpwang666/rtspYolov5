#ifndef _NVDEC_H
#define _NVDEC_H

#include "NvVideoDecoder.h"
#include "NvVideoConverter.h"
#include "NvEglRenderer.h"
#include <queue>
#include <fstream>
#include <pthread.h>
#include <semaphore.h>
#include "yolov5.h"

#define USE_NVBUF_TRANSFORM_API

#define MAX_BUFFERS 32




typedef struct
{
    NvVideoDecoder *dec;
    char decName[64];
    char DecCapPlane[64];
    char RendererName[64];
    NvVideoConverter *conv;
    uint32_t decoder_pixfmt;

    NvEglRenderer *renderer;

    yoloCuda_t yoloCuda;
   
    bool disable_rendering;
    bool fullscreen;
    uint32_t window_height;
    uint32_t window_width;
    uint32_t window_x;
    uint32_t window_y;
    uint32_t out_pixfmt;
    uint32_t video_height;
    uint32_t video_width;
    uint32_t display_height;
    uint32_t display_width;
    uint32_t qBuf_count;//初始化缓存计数 =2
    float fps;

    bool disable_dpb;

    bool input_nalu;

    bool copy_timestamp;
    bool flag_copyts;
    uint32_t start_ts;
    float dec_fps;
    uint64_t timestamp;
    uint64_t timestampincr;

    bool stats;

    int  stress_test;
    bool enable_metadata;
    bool bQueue;
    bool enable_input_metadata;
    enum v4l2_skip_frames_type skip_frames;
    enum v4l2_memory output_plane_mem_type;
    enum v4l2_memory capture_plane_mem_type;
#ifndef USE_NVBUF_TRANSFORM_API
    enum v4l2_yuv_rescale_method rescale_method;
#endif

    std::queue < NvBuffer * > *conv_output_plane_buf_queue;
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_cond;

    sem_t pollthread_sema; // Polling thread waits on this to be signalled to issue Poll
    sem_t decoderthread_sema; // Decoder thread waits on this to be signalled to continue q/dq loop
    pthread_t   dec_pollthread; // Polling thread, created if running in non-blocking mode.

    pthread_t dec_capture_loop; // Decoder capture thread, created if running in blocking mode.
    bool got_error;
    bool got_eos;
    bool vp9_file_header_flag;
    bool vp8_file_header_flag;
    int dst_dma_fd;
    int dmabuff_fd[MAX_BUFFERS];
    int numCapBuffers;
    int loop_count;
    int max_perf;
    int extra_cap_plane_buffer;
    int blocking_mode; // Set to true if running in blocking mode
    uint8_t chromaLocHoriz;
    uint8_t chromaLocVert;
} context_t;




void set_defaults(context_t * ctx,int x,int y,int width,int height);
void freeDecoder(context_t& ctx);
int initDecoder(context_t& ctx);
void initBuf(context_t& ctx,int index,u_char *in,int len);


#endif