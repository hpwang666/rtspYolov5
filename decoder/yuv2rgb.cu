/*
 * Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "NvUtils.h"
#include "NvBuffer.h"
#include "NvLogging.h"
#include "nvbuf_utils.h"
#include <stdio.h>
#include <cuda_runtime.h>
#include "yuv2rgb.cuh"

__device__ inline float clamp(float val, float mn, float mx)
{
	return (val >= mn)? ((val <= mx)? val : mx) : mn;
}

__global__ void gpuConvertYUYVtoRGB_kernel(unsigned char *src, unsigned char *dst,
		unsigned int width, unsigned int height)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx*2 >= width) {
		return;
	}

	for (int i = 0; i < height; ++i) {
		int y0 = src[i*width*2+idx*4+0];
		int cb = src[i*width*2+idx*4+1];
		int y1 = src[i*width*2+idx*4+2];
		int cr = src[i*width*2+idx*4+3];

		dst[i*width*3+idx*6+0] = clamp(1.164f * (y0 - 16) + 1.596f * (cr - 128)                      , 0.0f, 255.0f);
		dst[i*width*3+idx*6+1] = clamp(1.164f * (y0 - 16) - 0.813f * (cr - 128) - 0.391f * (cb - 128), 0.0f, 255.0f);
		dst[i*width*3+idx*6+2] = clamp(1.164f * (y0 - 16)                       + 2.018f * (cb - 128), 0.0f, 255.0f);

		dst[i*width*3+idx*6+3] = clamp(1.164f * (y1 - 16) + 1.596f * (cr - 128)                      , 0.0f, 255.0f);
		dst[i*width*3+idx*6+4] = clamp(1.164f * (y1 - 16) - 0.813f * (cr - 128) - 0.391f * (cb - 128), 0.0f, 255.0f);
		dst[i*width*3+idx*6+5] = clamp(1.164f * (y1 - 16)                       + 2.018f * (cb - 128), 0.0f, 255.0f);
	}
}


__global__ void gpuConvertYUYVtoRGB_kernel_(unsigned char *src, unsigned char *dst,
		unsigned int width, unsigned int height)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	 //int idx = (x) * 2;

	//for (int i = 0; i < height; ++i) {
		int y0 = src[idx*4+0];
		int cb = src[idx*4+1];
		int y1 = src[idx*4+2];
		int cr = src[idx*4+3];

		dst[idx*6+0] = clamp(1.164f * (y0 - 16) + 1.596f * (cr - 128)                      , 0.0f, 255.0f);
		dst[idx*6+1] = clamp(1.164f * (y0 - 16) - 0.813f * (cr - 128) - 0.391f * (cb - 128), 0.0f, 255.0f);
		dst[idx*6+2] = clamp(1.164f * (y0 - 16)                       + 2.018f * (cb - 128), 0.0f, 255.0f);

		dst[idx*6+3] = clamp(1.164f * (y1 - 16) + 1.596f * (cr - 128)                      , 0.0f, 255.0f);
		dst[idx*6+4] = clamp(1.164f * (y1 - 16) - 0.813f * (cr - 128) - 0.391f * (cb - 128), 0.0f, 255.0f);
		dst[idx*6+5] = clamp(1.164f * (y1 - 16)                       + 2.018f * (cb - 128), 0.0f, 255.0f);
	//}
}


void gpuConvertYUYVtoRGB(unsigned char *src, unsigned char *dst,
		unsigned int width, unsigned int height)
{
	unsigned char *d_src = src;
	unsigned char *d_dst = dst;
	size_t planeSize = width * height * sizeof(unsigned char);

	unsigned int flags;


	// if (srcIsMapped) {
	// 	printf("srcIsMapped\r\n");
	 	d_src = src;
	// 	cudaStreamAttachMemAsync(NULL, src, 0, cudaMemAttachGlobal);
	// } else {
	// 	//printf("srcIsnotMapped\r\n");
	// 	cudaMalloc(&d_src, planeSize * 2);
	// 	cudaMemcpy(d_src, src, planeSize * 2, cudaMemcpyHostToDevice);
	// }
	// if (dstIsMapped) {
	// 	printf("dstIsMapped\r\n");
		d_dst = dst;
	//	cudaStreamAttachMemAsync(NULL, dst, 0, cudaMemAttachGlobal);
	// } else {
	// 	//printf("dstIsnotMapped\r\n");
	// 	cudaMalloc(&d_dst, planeSize * 3);
	// }

	unsigned int blockSize = 1024;
	unsigned int numBlocks = (width / 2 + blockSize - 1) / blockSize;
	//gpuConvertYUYVtoRGB_kernel<<<numBlocks, blockSize>>>(d_src, d_dst, width, height);

    
    int jobs = height * width;
    int threads = 512;
    int blocks = ceil(jobs / (float)threads);
    
    // 启动核函数
   // rgb2bgr_kernel<<<gridDim, blockDim>>>(d_src, d_dst, width, height, 3);

    gpuConvertYUYVtoRGB_kernel_<<<blocks, threads>>>(d_src, d_dst, width, height);     

//	cudaStreamAttachMemAsync(NULL, dst, 0, cudaMemAttachHost);
	//cudaStreamSynchronize(NULL);
   // cudaDeviceSynchronize();

	// if (!srcIsMapped) {
	// 	cudaMemcpy(dst, d_dst, planeSize * 3, cudaMemcpyDeviceToHost);
	// 	cudaFree(d_src);
	// }
	// if (!dstIsMapped) {
	// 	cudaFree(d_dst);
	// }
}


int
copy_dmabuf2cuda(int dmabuf_fd,
                unsigned int plane,
                unsigned char* stream)
{
    if (dmabuf_fd <= 0)
        return -1;

    int ret = -1;
    NvBufferParams parm;
    int writeLen=0;
    ret = NvBufferGetParams(dmabuf_fd, &parm);

    if (ret != 0)
    {
        printf("GetParams failed \n");
        return -1;
    }

    void *psrc_data;

    ret = NvBufferMemMap(dmabuf_fd, plane, NvBufferMem_Read_Write, &psrc_data);
    if (ret == 0)
    {
        unsigned int i = 0;
        //NvBufferMemSyncForCpu(dmabuf_fd, plane, &psrc_data);
        //for (i = 0; i < parm.height[plane]; ++i)
        {
            if((parm.pixel_format == NvBufferColorFormat_NV12 ||
                parm.pixel_format == NvBufferColorFormat_NV16 ||
                parm.pixel_format == NvBufferColorFormat_NV24 ||
                parm.pixel_format == NvBufferColorFormat_NV12_ER ||
                parm.pixel_format == NvBufferColorFormat_NV12_709 ||
                parm.pixel_format == NvBufferColorFormat_NV12_709_ER ||
                parm.pixel_format == NvBufferColorFormat_NV12_2020) &&
                    plane == 1)
            {
                // memcpy(stream+writeLen,(char *)psrc_data + i * parm.pitch[plane],
                //                 parm.width[plane] * 2);
                cudaMemcpy(stream+writeLen, (char *)psrc_data + i * parm.pitch[plane],  parm.width[plane] * 2, cudaMemcpyHostToDevice);
                 writeLen+=parm.width[plane] * 2;
            }
            else if (parm.pixel_format == NvBufferColorFormat_YUYV)
            {
            //    memcpy(stream+writeLen,(char *)psrc_data + i * parm.pitch[plane],
            //                     parm.width[plane] * 2);
             cudaMemcpy(stream, (char *)psrc_data,  parm.height[plane]*parm.width[plane] * 2, cudaMemcpyHostToDevice);
                 writeLen+=parm.width[plane] * 2;
            }
            else
            {
                // memcpy(stream+writeLen,(char *)psrc_data + i * parm.pitch[plane],
                //                 parm.width[plane]);
                 cudaMemcpy(stream+writeLen, (char *)psrc_data + i * parm.pitch[plane],  parm.width[plane] * 2, cudaMemcpyHostToDevice);
                writeLen+=parm.width[plane] * 2;
            }
           
        }
        NvBufferMemUnMap(dmabuf_fd, plane, &psrc_data);
    }
    else
    {
        printf("NvBufferMap failed \n");
        return -1;
    }

    return writeLen;
}
