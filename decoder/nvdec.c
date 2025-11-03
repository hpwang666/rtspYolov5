

 /******************************************************************************

	Date        : 2020/5/1
    Author      : hpwang 
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>


#include "nvbuf_utils.h"
#include "NvApplicationProfiler.h"
#include "renderer.h"


#undef  _DEBUG
#define _DEBUG
#ifdef _DEBUG
	#define debug(...) printf(__VA_ARGS__)
#else
	#define debug(...)
#endif 

static void abort(context_t *ctx);
#define DECODER_ERROR(cond, str, label) if(cond) { \
                                        cerr << str << endl; \
                                        error = 1; \
                                        goto label; }

#define MICROSECOND_UNIT 1000000
#define CHUNK_SIZE 4000000
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#define IS_NAL_UNIT_START(buffer_ptr) (!buffer_ptr[0] && !buffer_ptr[1] && \
        !buffer_ptr[2] && (buffer_ptr[3] == 1))

#define IS_NAL_UNIT_START1(buffer_ptr) (!buffer_ptr[0] && !buffer_ptr[1] && \
        (buffer_ptr[2] == 1))

#define H264_NAL_UNIT_CODED_SLICE  1
#define H264_NAL_UNIT_CODED_SLICE_IDR  5

#define HEVC_NUT_TRAIL_N  0
#define HEVC_NUT_RASL_R  9
#define HEVC_NUT_BLA_W_LP  16
#define HEVC_NUT_CRA_NUT  21

#define IVF_FILE_HDR_SIZE   32
#define IVF_FRAME_HDR_SIZE  12

#define IS_H264_NAL_CODED_SLICE(buffer_ptr) ((buffer_ptr[0] & 0x1F) == H264_NAL_UNIT_CODED_SLICE)
#define IS_H264_NAL_CODED_SLICE_IDR(buffer_ptr) ((buffer_ptr[0] & 0x1F) == H264_NAL_UNIT_CODED_SLICE_IDR)

#define IS_MJPEG_START(buffer_ptr) (buffer_ptr[0] == 0xFF && buffer_ptr[1] == 0xD8)
#define IS_MJPEG_END(buffer_ptr) (buffer_ptr[0] == 0xFF && buffer_ptr[1] == 0xD9)

#define GET_H265_NAL_UNIT_TYPE(buffer_ptr) ((buffer_ptr[0] & 0x7E) >> 1)

using namespace std;

static int report_input_metadata(context_t *ctx, v4l2_ctrl_videodec_inputbuf_metadata *input_metadata);
static void *dec_capture_loop_fcn(void *arg);
static bool decoder_proc_blocking(context_t &ctx);

void freeDecoder(context_t& ctx);


void set_defaults(context_t * ctx,int x,int y,int width,int height)
{
    memset(ctx, 0, sizeof(context_t));
    ctx->decoder_pixfmt = V4L2_PIX_FMT_H264;
    ctx->input_nalu=1;
    ctx->stats=true;
    ctx->disable_rendering=false;
    ctx->fullscreen = false;
    ctx->window_height = height;
    ctx->window_width = width;
    ctx->window_x = x;
    ctx->window_y = y;
    ctx->out_pixfmt = 1;
    ctx->fps = 25;
    ctx->output_plane_mem_type = V4L2_MEMORY_MMAP;
    ctx->capture_plane_mem_type = V4L2_MEMORY_DMABUF;
    ctx->vp9_file_header_flag = 0;
    ctx->vp8_file_header_flag = 0;
    ctx->stress_test = 1;
    ctx->copy_timestamp = true;
    ctx->flag_copyts = false;
    ctx->start_ts = 0;
    ctx->dec_fps = 30;
    ctx->dst_dma_fd = -1;
    ctx->bQueue = false;
    ctx->loop_count = 0;
    ctx->max_perf = 0;
    ctx->extra_cap_plane_buffer = 1;
    ctx->blocking_mode = 1;
    ctx->qBuf_count=0;
#ifndef USE_NVBUF_TRANSFORM_API
    ctx->conv_output_plane_buf_queue = new queue < NvBuffer * >;
    ctx->rescale_method = V4L2_YUV_RESCALE_NONE;
#endif
    ctx->chromaLocHoriz = NVBUF_CHROMA_SUBSAMPLING_HORIZ_DEFAULT;
    ctx->chromaLocVert = NVBUF_CHROMA_SUBSAMPLING_VERT_DEFAULT;
    pthread_mutex_init(&ctx->queue_lock, NULL);
    pthread_cond_init(&ctx->queue_cond, NULL);
}


int initDecoder(context_t& ctx)
{
    
    int ret = 0;
    int error = 0;
    uint32_t i;

    printf("decname:%s\r\n",ctx.decName);
     
     /* Set thread name for decoder Output Plane thread. */
    //pthread_setname_np(pthread_self(), "DecOutPlane");

    //NvApplicationProfiler &profiler = NvApplicationProfiler::getProfilerInstance();


    /* Create NvVideoDecoder object for blocking or non-blocking I/O mode. */
    if (ctx.blocking_mode)
    {
        cout << "Creating decoder in blocking mode \n";
        ctx.dec = NvVideoDecoder::createVideoDecoder(ctx.decName);
    }
    else
    {
        cout << "Creating decoder in non-blocking mode \n";
        ctx.dec = NvVideoDecoder::createVideoDecoder(ctx.decName, O_NONBLOCK);
    }
    DECODER_ERROR(!ctx.dec, "Could not create decoder", cleanup);

   

    /* Enable profiling for decoder if stats are requested. */
    if (ctx.stats)
    {
        //profiler.start(NvApplicationProfiler::DefaultSamplingInterval);
        //ctx.dec->enableProfiling();
    }

    /* Subscribe to Resolution change event.
       Refer ioctl VIDIOC_SUBSCRIBE_EVENT */
    ret = ctx.dec->subscribeEvent(V4L2_EVENT_RESOLUTION_CHANGE, 0, 0);
    DECODER_ERROR(ret < 0, "Could not subscribe to V4L2_EVENT_RESOLUTION_CHANGE",
               cleanup);

    /* Set format on the output plane.
       Refer ioctl VIDIOC_S_FMT */
    ret = ctx.dec->setOutputPlaneFormat(ctx.decoder_pixfmt, CHUNK_SIZE);
    DECODER_ERROR(ret < 0, "Could not set output plane format", cleanup);

    /* Configure for frame input mode for decoder.
       Refer V4L2_CID_MPEG_VIDEO_DISABLE_COMPLETE_FRAME_INPUT */
    if (ctx.input_nalu)
    {
        /* Input to the decoder will be nal units. */
        printf("Setting frame input mode to 0 \n");
        ret = ctx.dec->setFrameInputMode(0);
        DECODER_ERROR(ret < 0,
                "Error in decoder setFrameInputMode", cleanup);
    }
    

    /* Disable decoder DPB management.
       NOTE: V4L2_CID_MPEG_VIDEO_DISABLE_DPB should be set after output plane
             set format */
    if (ctx.disable_dpb)
    {
        ret = ctx.dec->disableDPB();
        DECODER_ERROR(ret < 0, "Error in decoder disableDPB", cleanup);
    }

    /* Enable decoder error and metadata reporting.
       Refer V4L2_CID_MPEG_VIDEO_ERROR_REPORTING */
    if (ctx.enable_metadata || ctx.enable_input_metadata)
    {
        ret = ctx.dec->enableMetadataReporting();
        DECODER_ERROR(ret < 0, "Error while enabling metadata reporting", cleanup);
    }

    /* Enable max performance mode by using decoder max clock settings.
       Refer V4L2_CID_MPEG_VIDEO_MAX_PERFORMANCE */
    if (ctx.max_perf)
    {
        ret = ctx.dec->setMaxPerfMode(ctx.max_perf);
        DECODER_ERROR(ret < 0, "Error while setting decoder to max perf", cleanup);
    }

    /* Set the skip frames property of the decoder.
       Refer V4L2_CID_MPEG_VIDEO_SKIP_FRAMES */
    if (ctx.skip_frames)
    {
        ret = ctx.dec->setSkipFrames(ctx.skip_frames);
        DECODER_ERROR(ret < 0, "Error while setting skip frames param", cleanup);
    }

    /* Query, Export and Map the output plane buffers so can read
       encoded data into the buffers. */
    if (ctx.output_plane_mem_type == V4L2_MEMORY_MMAP) {
        /* configure decoder output plane for MMAP io-mode.
           Refer ioctl VIDIOC_REQBUFS, VIDIOC_QUERYBUF and VIDIOC_EXPBUF */
        ret = ctx.dec->output_plane.setupPlane(V4L2_MEMORY_MMAP, 2, true, false);
    } else if (ctx.output_plane_mem_type == V4L2_MEMORY_USERPTR) {
        /* configure decoder output plane for USERPTR io-mode.
           Refer ioctl VIDIOC_REQBUFS */
        ret = ctx.dec->output_plane.setupPlane(V4L2_MEMORY_USERPTR, 10, false, true);
    }
    DECODER_ERROR(ret < 0, "Error while setting up output plane", cleanup);


     /* Start stream processing on decoder output-plane.
       Refer ioctl VIDIOC_STREAMON */
    ret = ctx.dec->output_plane.setStreamStatus(true);
    DECODER_ERROR(ret < 0, "Error in output plane stream on", cleanup);

    /* Enable copy timestamp with start timestamp in seconds for decode fps.
       NOTE: Used to demonstrate how timestamp can be associated with an
             individual H264/H265 frame to achieve video-synchronization. */
    if (ctx.copy_timestamp && ctx.input_nalu) {
      ctx.timestamp = (ctx.start_ts * MICROSECOND_UNIT);
      ctx.timestampincr = (MICROSECOND_UNIT * 16) / ((uint32_t) (ctx.dec_fps * 16));
    }

    /* Read encoded data and enqueue all the output plane buffers.
       Exit loop in case file read is complete. */
    i = 0;

   

     /* Create threads for decoder output */
    if (ctx.blocking_mode)
    {
        pthread_create(&ctx.dec_capture_loop, NULL, dec_capture_loop_fcn, &ctx);
        /* Set thread name for decoder Capture Plane thread. */
        pthread_setname_np(ctx.dec_capture_loop, ctx.DecCapPlane);
    }
    else
    {
        // sem_init(&ctx.pollthread_sema, 0, 0);
        // sem_init(&ctx.decoderthread_sema, 0, 0);
        // pthread_create(&ctx.dec_pollthread, NULL, decoder_pollthread_fcn, &ctx);
        // cout << "Created the PollThread and Decoder Thread \n";
        // /* Set thread name for decoder Poll thread. */
        // pthread_setname_np(ctx.dec_pollthread, "DecPollThread");
        ;
    }

    // if (ctx.blocking_mode)
    //     decoder_proc_blocking(ctx);
    // else
    //     decoder_proc_blocking(ctx);
 

  

cleanup:
    //freeDecoder(ctx);
   

    return -error;

}
 
 void initBuf(context_t& ctx,int index,u_char *in,int len)
 {
    int ret;
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[MAX_PLANES];
    NvBuffer *buffer;
    u_char *stream_ptr = in +4;

    memset(&v4l2_buf, 0, sizeof(v4l2_buf));
    memset(planes, 0, sizeof(planes));

    buffer = ctx.dec->output_plane.getNthBuffer(index);
    
    memcpy((char *) buffer->planes[0].data,in,len);
    buffer->planes[0].bytesused =len;
    v4l2_buf.index = index;
    v4l2_buf.m.planes = planes;
    v4l2_buf.m.planes[0].bytesused = len;
    printf("bytesused= %d\r\n",v4l2_buf.m.planes[0].bytesused);

    if ((IS_H264_NAL_CODED_SLICE(stream_ptr)) ||
            (IS_H264_NAL_CODED_SLICE_IDR(stream_ptr)))
    {
        /* Update the timestamp. */
        v4l2_buf.flags |= V4L2_BUF_FLAG_TIMESTAMP_COPY;
        ctx.timestamp += 40000;
        v4l2_buf.timestamp.tv_sec = ctx.timestamp / (MICROSECOND_UNIT);
        v4l2_buf.timestamp.tv_usec = ctx.timestamp % (MICROSECOND_UNIT);
        printf("update timestamp:%lld\r\n",ctx.timestamp);
    }

    
    /* It is necessary to queue an empty buffer to signal EOS to the decoder
        i.e. set v4l2_buf.m.planes[0].bytesused = 0 and queue the buffer. */
    ret = ctx.dec->output_plane.qBuffer(v4l2_buf, NULL);
    if (ret < 0)
    {
        cerr << "Error Qing buffer at output plane" << endl;
        abort(&ctx);
    }
   
    
 }

void freeDecoder(context_t& ctx)
{
    int ret = 0;
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[4];
    NvBuffer *buffer;

    memset(&v4l2_buf, 0, sizeof(v4l2_buf));
    memset(planes, 0, sizeof(planes));

    v4l2_buf.m.planes = planes;

    ret = ctx.dec->output_plane.dqBuffer(v4l2_buf,&buffer, NULL, -1);
    if (ret < 0)
    {
        cerr << "Error DQing buffer at output plane" << endl;
        
    }
    /* It is necessary to queue an empty buffer to signal EOS to the decoder
           i.e. set v4l2_buf.m.planes[0].bytesused = 0 and queue the buffer. */
    v4l2_buf.m.planes[0].bytesused = buffer->planes[0].bytesused=0;
    ret = ctx.dec->output_plane.qBuffer(v4l2_buf, NULL);
    if (ret < 0)
    {
        cerr << "Error Qing buffer at output plane" << endl;
       
    }
    
       /* After sending EOS, all the buffers from output plane should be dequeued.
       and after that capture plane loop should be signalled to stop. */
    if (ctx.blocking_mode)
    {
        while (ctx.dec->output_plane.getNumQueuedBuffers() > 0 &&
               !ctx.got_error && !ctx.dec->isInError())
        {
           
            memset(&v4l2_buf, 0, sizeof(v4l2_buf));
            memset(planes, 0, sizeof(planes));

            v4l2_buf.m.planes = planes;
            ret = ctx.dec->output_plane.dqBuffer(v4l2_buf, NULL, NULL, -1);
            if (ret < 0)
            {
                cerr << "Error DQing buffer at output plane" << endl;
                abort(&ctx);
                break;
            }
            if (v4l2_buf.m.planes[0].bytesused == 0)
            {
                cout << "Got EoS at output plane"<< endl;
               // break;
            }
            printf("ctx.dec->output_plane.getNumQueuedBuffers():%d\r\n",ctx.dec->output_plane.getNumQueuedBuffers());

            if ((v4l2_buf.flags & V4L2_BUF_FLAG_ERROR) && ctx.enable_input_metadata)
            {
                v4l2_ctrl_videodec_inputbuf_metadata dec_input_metadata;
                /* Get the decoder input metadata.
                   Refer V4L2_CID_MPEG_VIDEODEC_INPUT_METADATA */
                ret = ctx.dec->getInputMetadata(v4l2_buf.index, dec_input_metadata);
                if (ret == 0)
                {
                    ret = report_input_metadata(&ctx, &dec_input_metadata);
                    if (ret == -1)
                    {
                      cerr << "Error with input stream header parsing" << endl;
                      abort(&ctx);
                      break;
                    }
                }
            }
        }
    }
    
    
    
    
    /* Signal EOS to the decoder capture loop. */
    ctx.got_eos = true;
#ifndef USE_NVBUF_TRANSFORM_API
    if (ctx.conv)
    {
        ctx.conv->capture_plane.waitForDQThread(-1);
    }
#endif

    
    if (ctx.blocking_mode && ctx.dec_capture_loop)
    {
        pthread_join(ctx.dec_capture_loop, NULL);
    }
    else if (!ctx.blocking_mode)
    {
        /* Clear the poll interrupt to get the decoder's poll thread out. */
        ctx.dec->ClearPollInterrupt();
        /* If Pollthread is waiting on, signal it to exit the thread. */
        sem_post(&ctx.pollthread_sema);
        pthread_join(ctx.dec_pollthread, NULL);
    }

    if (ctx.stats)
    {
        //profiler.stop();
        //ctx.dec->printProfilingStats(cout);
#ifndef USE_NVBUF_TRANSFORM_API
        if (ctx.conv)
        {
            ctx.conv->printProfilingStats(cout);
        }
#endif
        if (ctx.renderer)
        {
            //ctx.renderer->printProfilingStats(cout);
        }
        //profiler.printProfilerData(cout);
    }

    if(ctx.capture_plane_mem_type == V4L2_MEMORY_DMABUF)
    {
        for(int index = 0 ; index < ctx.numCapBuffers ; index++)
        {
            if(ctx.dmabuff_fd[index] != 0)
            {
                ret = NvBufferDestroy (ctx.dmabuff_fd[index]);
                if(ret < 0)
                {
                    cerr << "Failed to Destroy NvBuffer" << endl;
                }
            }
        }
    }
#ifndef USE_NVBUF_TRANSFORM_API
    if (ctx.conv && ctx.conv->isInError())
    {
        cerr << "Converter is in error" << endl;
        error = 1;
    }
#endif
    if (ctx.dec && ctx.dec->isInError())
    {
        cerr << "Decoder is in error" << endl;
    }

    if (ctx.got_error)
    {
         cerr << "Decoder is in error 1" << endl;
    }

    /* The decoder destructor does all the cleanup i.e set streamoff on output and
       capture planes, unmap buffers, tell decoder to deallocate buffer (reqbufs
       ioctl with count = 0), and finally call v4l2_close on the fd. */
    delete ctx.dec;
#ifndef USE_NVBUF_TRANSFORM_API
    delete ctx.conv;
#endif
    /* Similarly, EglRenderer destructor does all the cleanup. */
    delete ctx.renderer;
   
#ifndef USE_NVBUF_TRANSFORM_API
    delete ctx.conv_output_plane_buf_queue;
#else
    if(ctx.dst_dma_fd != -1)
    {
        NvBufferDestroy(ctx.dst_dma_fd);
        ctx.dst_dma_fd = -1;
    }
#endif

   
    if (!ctx.blocking_mode)
    {
        sem_destroy(&ctx.pollthread_sema);
        sem_destroy(&ctx.decoderthread_sema);
    }
}



static bool decoder_proc_blocking(context_t &ctx)
{
    int allow_DQ = true;
    int ret = 0;
    struct v4l2_buffer temp_buf;

    /* Since all the output plane buffers have been queued, we first need to
       dequeue a buffer from output plane before we can read new data into it
       and queue it again. */
    while (!ctx.got_eos && !ctx.got_error && !ctx.dec->isInError())
    {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];
        NvBuffer *buffer;

        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, sizeof(planes));

        v4l2_buf.m.planes = planes;

        /* dequeue a buffer for output plane. */
        if(allow_DQ)
        {
            ret = ctx.dec->output_plane.dqBuffer(v4l2_buf, &buffer, NULL, -1);
            if (ret < 0)
            {
                cerr << "Error DQing buffer at output plane" << endl;
                abort(&ctx);
                break;
            }
        }
        else
        {
            allow_DQ = true;
            memcpy(&v4l2_buf,&temp_buf,sizeof(v4l2_buffer));
            buffer = ctx.dec->output_plane.getNthBuffer(v4l2_buf.index);
        }

        if ((v4l2_buf.flags & V4L2_BUF_FLAG_ERROR) && ctx.enable_input_metadata)
        {
            v4l2_ctrl_videodec_inputbuf_metadata dec_input_metadata;

            /* Get the decoder input metadata.
               Refer V4L2_CID_MPEG_VIDEODEC_INPUT_METADATA */
            ret = ctx.dec->getInputMetadata(v4l2_buf.index, dec_input_metadata);
            if (ret == 0)
            {
                ret = report_input_metadata(&ctx, &dec_input_metadata);
                if (ret == -1)
                {
                  cerr << "Error with input stream header parsing" << endl;
                }
            }
        }

        if ((ctx.decoder_pixfmt == V4L2_PIX_FMT_H264) ||
                (ctx.decoder_pixfmt == V4L2_PIX_FMT_H265) ||
                (ctx.decoder_pixfmt == V4L2_PIX_FMT_MPEG2) ||
                (ctx.decoder_pixfmt == V4L2_PIX_FMT_MPEG4))
        {
            if (ctx.input_nalu)
            {
                /* read the input nal unit. */
                // read_decoder_input_nalu(ctx.in_file[current_file], buffer, nalu_parse_buffer,
                //         CHUNK_SIZE, &ctx);
            }
           
        }

       

        v4l2_buf.m.planes[0].bytesused = buffer->planes[0].bytesused;
        printf("bytesused= %d\r\n",v4l2_buf.m.planes[0].bytesused);
        if (ctx.input_nalu && ctx.copy_timestamp && ctx.flag_copyts)
        {
          /* Update the timestamp. */
          v4l2_buf.flags |= V4L2_BUF_FLAG_TIMESTAMP_COPY;
          ctx.timestamp += ctx.timestampincr;
          v4l2_buf.timestamp.tv_sec = ctx.timestamp / (MICROSECOND_UNIT);
          v4l2_buf.timestamp.tv_usec = ctx.timestamp % (MICROSECOND_UNIT);
        }

       

        /* enqueue a buffer for output plane. */
        ret = ctx.dec->output_plane.qBuffer(v4l2_buf, NULL);
        if (ret < 0)
        {
            cerr << "Error Qing buffer at output plane" << endl;
            abort(&ctx);
            break;
        }
        
    }
    return 0;
}

static void *dec_capture_loop_fcn(void *arg)
{
    context_t *ctx = (context_t *) arg;
    NvVideoDecoder *dec = ctx->dec;
    struct v4l2_event ev;
    int ret;

    cout << "Starting decoder capture loop thread" << endl;
    /* Need to wait for the first Resolution change event, so that
       the decoder knows the stream resolution and can allocate appropriate
       buffers when we call REQBUFS. */
    do
    {
        /* Refer ioctl VIDIOC_DQEVENT */
        ret = dec->dqEvent(ev, 50000);
        if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                cerr <<
                    "Timed out waiting for first V4L2_EVENT_RESOLUTION_CHANGE"
                    << endl;
            }
            else
            {
                cerr << "Error in dequeueing decoder event" << endl;
            }
            abort(ctx);
            break;
        }
    }
    while ((ev.type != V4L2_EVENT_RESOLUTION_CHANGE) && !ctx->got_error);

    /* Received the resolution change event, now can do query_and_set_capture. */
    if (!ctx->got_error)
        query_and_set_capture(ctx);

    /* Exit on error or EOS which is signalled in main() */
   // while (!(ctx->got_error || dec->isInError()))
    while (!ctx->got_eos && !ctx->got_error && !dec->isInError())
    {
        NvBuffer *dec_buffer;

        /* Check for Resolution change again.
           Refer ioctl VIDIOC_DQEVENT */
        ret = dec->dqEvent(ev, false);
        if (ret == 0)
        {
            switch (ev.type)
            {
                case V4L2_EVENT_RESOLUTION_CHANGE:
                    query_and_set_capture(ctx);
                    continue;
            }
        }

        /* Decoder capture loop */
        while (1)
        {
            struct v4l2_buffer v4l2_buf;
            struct v4l2_plane planes[MAX_PLANES];

            memset(&v4l2_buf, 0, sizeof(v4l2_buf));
            memset(planes, 0, sizeof(planes));
            v4l2_buf.m.planes = planes;

            /* Dequeue a filled buffer. */
            if (dec->capture_plane.dqBuffer(v4l2_buf, &dec_buffer, NULL, 0))
            {
                if (errno == EAGAIN)
                {
                    if (v4l2_buf.flags & V4L2_BUF_FLAG_LAST)
                    {
                        cout << "Got EoS at capture plane" << endl;
                        goto handle_eos;
                    }
                    usleep(1000);
                }
                else
                {
                    abort(ctx);
                    cerr << "Error while calling dequeue at capture plane" <<
                        endl;
                }
                break;
            }

            if (ctx->enable_metadata)
            {
                v4l2_ctrl_videodec_outputbuf_metadata dec_metadata;

                /* Get the decoder output metadata on capture-plane.
                   Refer V4L2_CID_MPEG_VIDEODEC_METADATA */
                ret = dec->getMetadata(v4l2_buf.index, dec_metadata);
                if (ret == 0)
                {
                    report_metadata(ctx, &dec_metadata);
                }
            }

            if (false&&ctx->copy_timestamp && ctx->input_nalu && ctx->stats)
            {
              cout << "[" << v4l2_buf.index << "]" "dec capture plane dqB timestamp [" <<
                  v4l2_buf.timestamp.tv_sec << "s" << v4l2_buf.timestamp.tv_usec << "us]" << endl;
            }

            if (!ctx->disable_rendering && ctx->stats)
            {
                /* EglRenderer requires the fd of the 0th plane to render the buffer. */
                if(ctx->capture_plane_mem_type == V4L2_MEMORY_DMABUF)
                    dec_buffer->planes[0].fd = ctx->dmabuff_fd[v4l2_buf.index];
                ctx->renderer->render(dec_buffer->planes[0].fd);
            }

            /* If we need to write to file or display the buffer, give
               the buffer to video converter output plane instead of
               returning the buffer back to decoder capture plane. */

            
            
            /* If not writing to file, Queue the buffer back once it has been used. */
            if(ctx->capture_plane_mem_type == V4L2_MEMORY_DMABUF)
                v4l2_buf.m.planes[0].m.fd = ctx->dmabuff_fd[v4l2_buf.index];
            if (dec->capture_plane.qBuffer(v4l2_buf, NULL) < 0)
            {
                abort(ctx);
                cerr <<
                    "Error while queueing buffer at decoder capture plane"
                    << endl;
                break;
            }
            
        }
    }
handle_eos:

#ifndef USE_NVBUF_TRANSFORM_API
    /* Send EOS to converter */
    if (ctx->conv)
    {
        if (sendEOStoConverter(ctx) < 0)
        {
            cerr << "Error while queueing EOS buffer on converter output"
                 << endl;
        }
    }
#endif
    cout << "Exiting decoder capture loop thread" << endl;
    return NULL;
}



static int report_input_metadata(context_t *ctx, v4l2_ctrl_videodec_inputbuf_metadata *input_metadata)
{
    int ret = -1;
    uint32_t frame_num = ctx->dec->output_plane.getTotalDequeuedBuffers() - 1;

    /* NOTE: Bits represent types of error as defined with v4l2_videodec_input_error_type. */
    if (input_metadata->nBitStreamError & V4L2_DEC_ERROR_SPS) {
      cout << "Frame " << frame_num << " BitStreamError : ERROR_SPS " << endl;
    } else if (input_metadata->nBitStreamError & V4L2_DEC_ERROR_PPS) {
      cout << "Frame " << frame_num << " BitStreamError : ERROR_PPS " << endl;
    } else if (input_metadata->nBitStreamError & V4L2_DEC_ERROR_SLICE_HDR) {
      cout << "Frame " << frame_num << " BitStreamError : ERROR_SLICE_HDR " << endl;
    } else if (input_metadata->nBitStreamError & V4L2_DEC_ERROR_MISSING_REF_FRAME) {
      cout << "Frame " << frame_num << " BitStreamError : ERROR_MISSING_REF_FRAME " << endl;
    } else if (input_metadata->nBitStreamError & V4L2_DEC_ERROR_VPS) {
      cout << "Frame " << frame_num << " BitStreamError : ERROR_VPS " << endl;
    } else {
      cout << "Frame " << frame_num << " BitStreamError : ERROR_None " << endl;
      ret = 0;
    }
    return ret;
}

static void
abort(context_t *ctx)
{
    ctx->got_error = true;
    ctx->dec->abort();
#ifndef USE_NVBUF_TRANSFORM_API
    if (ctx->conv)
    {
        ctx->conv->abort();
        pthread_cond_broadcast(&ctx->queue_cond);
    }
#endif
}