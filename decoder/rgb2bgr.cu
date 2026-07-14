#include <cuda_runtime.h>
#include <cstdint>

__global__ void rgb2bgr_kernel(unsigned char* src, unsigned char* dst, 
                               int width, int height, int channels) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    uint8_t t;
    if (x < width && y < height) {
        int idx = (y * width + x) * channels;
        
        // RGB -> BGR 交换R和B通道
        t = src[idx + 2];     // B = R
        //dst[idx + 1] = src[idx + 1]; // G保持不变
        src[idx + 2] = src[idx];     // R = B
        src[idx] =t;
    }
}

__global__ void rgb2bgr_kernel_(unsigned char* src, unsigned char* dst, 
                               int width, int height, int channels) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
   // int y = blockIdx.y * blockDim.y + threadIdx.y;
    uint8_t t;
    if (x < width* height) {
        int idx = (x) * channels;
        
        // RGB -> BGR 交换R和B通道
        t = src[idx + 2];     // B = R
        //dst[idx + 1] = src[idx + 1]; // G保持不变
        src[idx + 2] = src[idx];     // R = B
        src[idx] =t;
    }
}

void rgb2bgr_cuda(unsigned char *src, unsigned char *dst,
		unsigned int width, unsigned int height,cudaStream_t stream) {
    // 确保是3通道RGB图像
    unsigned char *d_src = src;
	unsigned char *d_dst =dst;
   
    // cudaMalloc(&d_dst,image_size );
    
   // cudaStreamAttachMemAsync(NULL, src, 0, cudaMemAttachGlobal);
    // 配置线程块和网格
    // dim3 blockDim(16, 16);
    // dim3 gridDim((width + blockDim.x - 1) / blockDim.x,
    //              (height + blockDim.y - 1) / blockDim.y);

    int jobs = height * width;
    int threads = 64;
    int blocks = ceil(jobs / (float)threads);
    
    // 启动核函数
   // rgb2bgr_kernel<<<gridDim, blockDim>>>(d_src, d_dst, width, height, 3);

    rgb2bgr_kernel_<<<blocks, threads,0,stream>>>(d_src, d_dst, width, height, 3);                                      
    
    //cudaStreamAttachMemAsync(NULL, dst, 0, cudaMemAttachHost);
	//cudaStreamSynchronize(NULL);
//cudaMemcpy(dst, d_dst, image_size, cudaMemcpyDeviceToHost);

  
    
    // 清理设备内存
 //   cudaFree(d_dst);
   
}