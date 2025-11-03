#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <linux/videodev2.h>


#include "nvbuf_utils.h"
#include "NvEglRenderer.h"
#include "nvdec.h"


#define RENDERER_ERROR(cond, str, label) if(cond) { \
                                        cerr << str << endl; \
                                        error = 1; \
                                        goto label; }


using namespace std;

void query_and_set_capture(context_t * ctx)
{
    NvVideoDecoder *dec = ctx->dec;
    struct v4l2_format format;
    struct v4l2_crop crop;
    int32_t min_dec_capture_buffers;
    int ret = 0;
    int error = 0;
    uint32_t window_width;
    uint32_t window_height;
    uint32_t sar_width;
    uint32_t sar_height;
    NvBufferCreateParams input_params = {0};
    NvBufferCreateParams cParams = {0};
    NvBufferChromaSubsamplingParams outChromaSubsampling = {ctx->chromaLocHoriz, ctx->chromaLocVert};
#ifndef USE_NVBUF_TRANSFORM_API
    uint32_t pixfmt = 0;
#endif

    /* Get capture plane format from the decoder.
       This may change after resolution change event.
       Refer ioctl VIDIOC_G_FMT */
    ret = dec->capture_plane.getFormat(format);
    RENDERER_ERROR(ret < 0,
               "Error: Could not get format from decoder capture plane", error);

    /* Get the display resolution from the decoder.
       Refer ioctl VIDIOC_G_CROP */
    ret = dec->capture_plane.getCrop(crop);
    RENDERER_ERROR(ret < 0,
               "Error: Could not get crop from decoder capture plane", error);

    cout << "Video Resolution: " << crop.c.width << "x" << crop.c.height
        << endl;
    ctx->display_height = crop.c.height;
    ctx->display_width = crop.c.width;

    /* Get the Sample Aspect Ratio (SAR) width and height */
    ret = dec->getSAR(sar_width, sar_height);
    cout << "Video SAR width: " << sar_width << " SAR height: " << sar_height << endl;
#ifdef USE_NVBUF_TRANSFORM_API
    if(ctx->dst_dma_fd != -1)
    {
        NvBufferDestroy(ctx->dst_dma_fd);
        ctx->dst_dma_fd = -1;
    }
    /* Create PitchLinear output buffer for transform. */
    input_params.payloadType = NvBufferPayload_SurfArray;
    input_params.width = crop.c.width;
    input_params.height = crop.c.height;
    input_params.layout = NvBufferLayout_Pitch;
    if (ctx->out_pixfmt == 1)
      input_params.colorFormat = NvBufferColorFormat_NV12;
    else if (ctx->out_pixfmt == 2)
      input_params.colorFormat = NvBufferColorFormat_YUV420;
    else if (ctx->out_pixfmt == 3)
      input_params.colorFormat = NvBufferColorFormat_NV16;
    else if (ctx->out_pixfmt == 4)
      input_params.colorFormat = NvBufferColorFormat_NV24;
    else if (ctx->out_pixfmt == 5)
      input_params.colorFormat = NvBufferColorFormat_YUV422;
    else if (ctx->out_pixfmt == 6)
      input_params.colorFormat = NvBufferColorFormat_YUV444;
    else if (ctx->out_pixfmt == 7)
      input_params.colorFormat = NvBufferColorFormat_YUYV;

    input_params.nvbuf_tag = NvBufferTag_VIDEO_CONVERT;

    ret = NvBufferCreateWithChromaLoc (&ctx->dst_dma_fd, &input_params, &outChromaSubsampling);
    RENDERER_ERROR(ret == -1, "create dmabuf failed", error);
#else
    /* For file write, first deinitialize output and capture planes
       of video converter and then use the new resolution from
       decoder event resolution change. */
    if (ctx->conv)
    {
        ret = sendEOStoConverter(ctx);
        RENDERER_ERROR(ret < 0,
                   "Error while queueing EOS buffer on converter output",
                   error);

        ctx->conv->capture_plane.waitForDQThread(2000);

        ctx->conv->output_plane.deinitPlane();
        ctx->conv->capture_plane.deinitPlane();

        while(!ctx->conv_output_plane_buf_queue->empty())
        {
            ctx->conv_output_plane_buf_queue->pop();
        }
    }
#endif

    if (!ctx->disable_rendering)
    {
        /* Destroy the old instance of renderer as resolution might have changed. */
        delete ctx->renderer;

        if (ctx->fullscreen)
        {
            /* Required for fullscreen. */
            window_width = window_height = 0;
        }
        else if (ctx->window_width && ctx->window_height)
        {
            /* As specified by user on commandline. */
            window_width = ctx->window_width;
            window_height = ctx->window_height;
        }
        else
        {
            /* Resolution got from the decoder. */
            window_width = crop.c.width;
            window_height = crop.c.height;
        }

        /* If height or width are set to zero, EglRenderer creates a fullscreen
           window for rendering. */
           
        ctx->renderer =
                NvEglRenderer::createEglRenderer(ctx->RendererName, window_width,
                                           window_height, ctx->window_x,
                                           ctx->window_y);

                                           
      
        RENDERER_ERROR(!ctx->renderer,
                   "Error in setting up renderer. "
                   "Check if X is running or run with --disable-rendering",
                   error);


        // RENDERER_ERROR(!ctx->renderer_1,
        // "Error in setting up renderer_1. "
        // "Check if X is running or run with --disable-rendering",
        // error);
        if (ctx->stats)
        {
            /* Enable profiling for renderer if stats are requested. */
            ctx->renderer->enableProfiling();
        }

        /* Set fps for rendering. */
        ctx->renderer->setFPS(ctx->fps);

    }

    /* deinitPlane unmaps the buffers and calls REQBUFS with count 0 */
    dec->capture_plane.deinitPlane();
    if(ctx->capture_plane_mem_type == V4L2_MEMORY_DMABUF)
    {
        for(int index = 0 ; index < ctx->numCapBuffers ; index++)
        {
            if(ctx->dmabuff_fd[index] != 0)
            {
                ret = NvBufferDestroy (ctx->dmabuff_fd[index]);
                RENDERER_ERROR(ret < 0, "Failed to Destroy NvBuffer", error);
            }
        }
    }

    /* Not necessary to call VIDIOC_S_FMT on decoder capture plane.
       But decoder setCapturePlaneFormat function updates the class variables */
    ret = dec->setCapturePlaneFormat(format.fmt.pix_mp.pixelformat,
                                     format.fmt.pix_mp.width,
                                     format.fmt.pix_mp.height);
    RENDERER_ERROR(ret < 0, "Error in setting decoder capture plane format", error);

    ctx->video_height = format.fmt.pix_mp.height;
    ctx->video_width = format.fmt.pix_mp.width;
    /* Get the minimum buffers which have to be requested on the capture plane. */
    ret = dec->getMinimumCapturePlaneBuffers(min_dec_capture_buffers);
    RENDERER_ERROR(ret < 0,
               "Error while getting value of minimum capture plane buffers",
               error);

    /* Request (min + extra) buffers, export and map buffers. */
    if(ctx->capture_plane_mem_type == V4L2_MEMORY_MMAP)
    {
        /* Request, Query and export decoder capture plane buffers.
           Refer ioctl VIDIOC_REQBUFS, VIDIOC_QUERYBUF and VIDIOC_EXPBUF */
        ret =
            dec->capture_plane.setupPlane(V4L2_MEMORY_MMAP,
                                          min_dec_capture_buffers + ctx->extra_cap_plane_buffer, false,
                                          false);
        RENDERER_ERROR(ret < 0, "Error in decoder capture plane setup", error);
    }
    else if(ctx->capture_plane_mem_type == V4L2_MEMORY_DMABUF)
    {
        /* Set colorformats for relevant colorspaces. */
        switch(format.fmt.pix_mp.colorspace)
        {
            case V4L2_COLORSPACE_SMPTE170M:
                if (format.fmt.pix_mp.quantization == V4L2_QUANTIZATION_DEFAULT)
                {
                    cout << "Decoder colorspace ITU-R BT.601 with standard range luma (16-235)" << endl;
                    cParams.colorFormat = NvBufferColorFormat_NV12;
                }
                else
                {
                    cout << "Decoder colorspace ITU-R BT.601 with extended range luma (0-255)" << endl;
                    cParams.colorFormat = NvBufferColorFormat_NV12_ER;
                }
                break;
            case V4L2_COLORSPACE_REC709:
                if (format.fmt.pix_mp.quantization == V4L2_QUANTIZATION_DEFAULT)
                {
                    cout << "Decoder colorspace ITU-R BT.709 with standard range luma (16-235)" << endl;
                    cParams.colorFormat = NvBufferColorFormat_NV12_709;
                }
                else
                {
                    cout << "Decoder colorspace ITU-R BT.709 with extended range luma (0-255)" << endl;
                    cParams.colorFormat = NvBufferColorFormat_NV12_709_ER;
                }
                break;
            case V4L2_COLORSPACE_BT2020:
                {
                    cout << "Decoder colorspace ITU-R BT.2020" << endl;
                    cParams.colorFormat = NvBufferColorFormat_NV12_2020;
                }
                break;
            default:
                cout << "supported colorspace details not available, use default" << endl;
                if (format.fmt.pix_mp.quantization == V4L2_QUANTIZATION_DEFAULT)
                {
                    cout << "Decoder colorspace ITU-R BT.601 with standard range luma (16-235)" << endl;
                    cParams.colorFormat = NvBufferColorFormat_NV12;
                }
                else
                {
                    cout << "Decoder colorspace ITU-R BT.601 with extended range luma (0-255)" << endl;
                    cParams.colorFormat = NvBufferColorFormat_NV12_ER;
                }
                break;
        }

        ctx->numCapBuffers = min_dec_capture_buffers + ctx->extra_cap_plane_buffer;

        cParams.width = crop.c.width;
        cParams.height = crop.c.height;
        cParams.layout = NvBufferLayout_BlockLinear;
        cParams.payloadType = NvBufferPayload_SurfArray;
        cParams.nvbuf_tag = NvBufferTag_VIDEO_DEC;

        if (format.fmt.pix_mp.pixelformat  == V4L2_PIX_FMT_NV24M)
            cParams.colorFormat = NvBufferColorFormat_NV24;
        else if (format.fmt.pix_mp.pixelformat  == V4L2_PIX_FMT_NV24_10LE)
            cParams.colorFormat = NvBufferColorFormat_NV24_10LE;
        if (ctx->decoder_pixfmt == V4L2_PIX_FMT_MJPEG)
        {
            cParams.layout = NvBufferLayout_Pitch;
            cParams.nvbuf_tag = NvBufferTag_JPEG;
            if (format.fmt.pix_mp.pixelformat == V4L2_PIX_FMT_YUV422M)
            {
                cParams.colorFormat = NvBufferColorFormat_YUV422;
            }
            else
            {
                cParams.colorFormat = NvBufferColorFormat_YUV420;
            }
        }

        /* Create decoder capture plane buffers. */
        for (int index = 0; index < ctx->numCapBuffers; index++)
        {
            ret = NvBufferCreateEx(&ctx->dmabuff_fd[index], &cParams);
            RENDERER_ERROR(ret < 0, "Failed to create buffers", error);
        }

        /* Request buffers on decoder capture plane.
           Refer ioctl VIDIOC_REQBUFS */
        ret = dec->capture_plane.reqbufs(V4L2_MEMORY_DMABUF,ctx->numCapBuffers);
            RENDERER_ERROR(ret, "Error in request buffers on capture plane", error);
    }

#ifndef USE_NVBUF_TRANSFORM_API
    if (ctx->conv)
    {
        /* Set Converter output plane format.
           Refer ioctl VIDIOC_S_FMT */
        ret = ctx->conv->setOutputPlaneFormat(format.fmt.pix_mp.pixelformat,
                                              format.fmt.pix_mp.width,
                                              format.fmt.pix_mp.height,
                                              V4L2_NV_BUFFER_LAYOUT_BLOCKLINEAR);
        RENDERER_ERROR(ret < 0, "Error in converter output plane set format",
                   error);

        if ((ctx->out_pixfmt == 5 || ctx->out_pixfmt == 6 || ctx->out_pixfmt == 7) &&
             ctx->decoder_pixfmt != V4L2_PIX_FMT_MJPEG)
        {
            cout << "Can use -f " << ctx->out_pixfmt << " only with MJPEG format\n";
            cout << "Changing output pixel format to NV12[default]\n";
            ctx->out_pixfmt = 1;
        }

        if (ctx->out_pixfmt == 1)
        {
            pixfmt = V4L2_PIX_FMT_NV12M;
        }
        else if (ctx->out_pixfmt == 5)
        {
            pixfmt = V4L2_PIX_FMT_YUV422M;
        }
        else if (ctx->out_pixfmt == 6)
        {
            pixfmt = V4L2_PIX_FMT_YUV444M;
        }
        else if (ctx->out_pixfmt == 7)
        {
            pixfmt = V4L2_PIX_FMT_YUYV;
        }
        else
        {
            pixfmt = V4L2_PIX_FMT_YUV420M;
        }

        /* Set Converter capture plane format.
           Refer ioctl VIDIOC_S_FMT */
        ret = ctx->conv->setCapturePlaneFormat(pixfmt, crop.c.width,
                                                crop.c.height,
                                                V4L2_NV_BUFFER_LAYOUT_PITCH);
        RENDERER_ERROR(ret < 0, "Error in converter capture plane set format",
                   error);

        /* Set Converter crop rectangle. */
        ret = ctx->conv->setCropRect(0, 0, crop.c.width, crop.c.height);
        RENDERER_ERROR(ret < 0, "Error while setting crop rect", error);

        if (ctx->rescale_method) {
            /* Rescale full range [0-255] to limited range [16-235].
               Refer V4L2_CID_VIDEO_CONVERT_YUV_RESCALE_METHOD */
            ret = ctx->conv->setYUVRescale(ctx->rescale_method);
            RENDERER_ERROR(ret < 0, "Error while setting YUV rescale", error);
        }

        /* Request buffers on converter output plane.
           Refer ioctl VIDIOC_REQBUFS */
        ret =
            ctx->conv->output_plane.setupPlane(V4L2_MEMORY_DMABUF,
                                                dec->capture_plane.
                                                getNumBuffers(), false, false);
        RENDERER_ERROR(ret < 0, "Error in converter output plane setup", error);

        /* Request, Query and export converter capture plane buffers.
           Refer ioctl VIDIOC_REQBUFS, VIDIOC_QUERYBUF and VIDIOC_EXPBUF */
        ret =
            ctx->conv->capture_plane.setupPlane(V4L2_MEMORY_MMAP,
                                                 dec->capture_plane.
                                                 getNumBuffers(), true, false);
        RENDERER_ERROR(ret < 0, "Error in converter capture plane setup", error);

        /* Converter output plane STREAMON.
           Refer ioctl VIDIOC_STREAMON */
        ret = ctx->conv->output_plane.setStreamStatus(true);
        RENDERER_ERROR(ret < 0, "Error in converter output plane streamon", error);

        /* Converter capture plane STREAMON.
           Refer ioctl VIDIOC_STREAMON */
        ret = ctx->conv->capture_plane.setStreamStatus(true);
        RENDERER_ERROR(ret < 0, "Error in converter output plane streamoff", error);

        /* Add all empty conv output plane buffers to conv_output_plane_buf_queue. */
        for (uint32_t i = 0; i < ctx->conv->output_plane.getNumBuffers(); i++)
        {
            ctx->conv_output_plane_buf_queue->push(ctx->conv->output_plane.
                    getNthBuffer(i));
        }

        /* Enqueue converter capture plane buffers. */
        for (uint32_t i = 0; i < ctx->conv->capture_plane.getNumBuffers(); i++)
        {
            struct v4l2_buffer v4l2_buf;
            struct v4l2_plane planes[MAX_PLANES];

            memset(&v4l2_buf, 0, sizeof(v4l2_buf));
            memset(planes, 0, sizeof(planes));

            v4l2_buf.index = i;
            v4l2_buf.m.planes = planes;
            ret = ctx->conv->capture_plane.qBuffer(v4l2_buf, NULL);
            RENDERER_ERROR(ret < 0, "Error Qing buffer at converter capture plane",
                       error);
        }
        /* Start deque thread for converter output plane. */
        ctx->conv->output_plane.startDQThread(ctx);
        /* Start deque thread for converter capture plane. */
        ctx->conv->capture_plane.startDQThread(ctx);
    }
#endif

    /* Decoder capture plane STREAMON.
       Refer ioctl VIDIOC_STREAMON */
    ret = dec->capture_plane.setStreamStatus(true);
    RENDERER_ERROR(ret < 0, "Error in decoder capture plane streamon", error);

    /* Enqueue all the empty decoder capture plane buffers. */
    for (uint32_t i = 0; i < dec->capture_plane.getNumBuffers(); i++)
    {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];

        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, sizeof(planes));

        v4l2_buf.index = i;
        v4l2_buf.m.planes = planes;
        v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.memory = ctx->capture_plane_mem_type;
        if(ctx->capture_plane_mem_type == V4L2_MEMORY_DMABUF)
            v4l2_buf.m.planes[0].m.fd = ctx->dmabuff_fd[i];
        ret = dec->capture_plane.qBuffer(v4l2_buf, NULL);
        RENDERER_ERROR(ret < 0, "Error Qing buffer at output plane", error);
    }
    cout << "Query and set capture successful" << endl;
    return;

error:
    if (error)
    {
        cerr << "Error in " << __func__ << endl;
    }
}




 void report_metadata(context_t *ctx, v4l2_ctrl_videodec_outputbuf_metadata *metadata)
{
    uint32_t frame_num = ctx->dec->capture_plane.getTotalDequeuedBuffers() - 1;

    cout << "Frame " << frame_num << endl;

    if (metadata->bValidFrameStatus)
    {
        if (ctx->decoder_pixfmt == V4L2_PIX_FMT_H264)
        {
            /* metadata for H264 input stream. */
            switch(metadata->CodecParams.H264DecParams.FrameType)
            {
                case 0:
                    cout << "FrameType = B" << endl;
                    break;
                case 1:
                    cout << "FrameType = P" << endl;
                    break;
                case 2:
                    cout << "FrameType = I";
                    if (metadata->CodecParams.H264DecParams.dpbInfo.currentFrame.bIdrFrame)
                    {
                        cout << " (IDR)";
                    }
                    cout << endl;
                    break;
            }
            cout << "nActiveRefFrames = " << metadata->CodecParams.H264DecParams.dpbInfo.nActiveRefFrames << endl;
        }

        if (ctx->decoder_pixfmt == V4L2_PIX_FMT_H265)
        {
            /* metadata for HEVC input stream. */
            switch(metadata->CodecParams.HEVCDecParams.FrameType)
            {
                case 0:
                    cout << "FrameType = B" << endl;
                    break;
                case 1:
                    cout << "FrameType = P" << endl;
                    break;
                case 2:
                    cout << "FrameType = I";
                    if (metadata->CodecParams.HEVCDecParams.dpbInfo.currentFrame.bIdrFrame)
                    {
                        cout << " (IDR)";
                    }
                    cout << endl;
                    break;
            }
            cout << "nActiveRefFrames = " << metadata->CodecParams.HEVCDecParams.dpbInfo.nActiveRefFrames << endl;
        }

        if (metadata->FrameDecStats.DecodeError)
        {
            /* decoder error status metadata. */
            v4l2_ctrl_videodec_statusmetadata *dec_stats =
                &metadata->FrameDecStats;
            cout << "ErrorType="  << dec_stats->DecodeError << " Decoded MBs=" <<
                dec_stats->DecodedMBs << " Concealed MBs=" <<
                dec_stats->ConcealedMBs << endl;
        }
    }
    else
    {
        cout << "No valid metadata for frame" << endl;
    }
}