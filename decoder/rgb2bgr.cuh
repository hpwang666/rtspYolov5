#ifndef __RGB2BGR_CUH__
#define __RGB2BGR_CUH__
#include <cuda_runtime.h>

void rgb2bgr_cuda(unsigned char *src, unsigned char *dst,
		unsigned int width, unsigned int height,cudaStream_t stream);

#endif