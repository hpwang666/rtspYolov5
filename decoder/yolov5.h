
#ifndef _YOLOV5_H
#define _YOLOV5_H



#include "logging.h"
#include "yololayer.h"



#define USE_FP32  // set USE_INT8 or USE_FP16 or USE_FP32
#define DEVICE 0  // GPU id
#define NMS_THRESH 0.4 //0.4
#define CONF_THRESH 0.5	//置信度，默认值为0.5，由于效果不好修改为0.25取得了较好的效果
#define BATCH_SIZE 1
 


using namespace nvinfer1;
typedef struct {
    //cuda
    float *prob;
    IRuntime* runtime;
    ICudaEngine* engine;
    IExecutionContext* context;
    Logger gLogger;
    cudaStream_t stream;
    void* buffers[2];
    int inputIndex;
    int outputIndex;
    unsigned char *rgb_out_buffer;
    
}yoloCuda_t;


void initCuda(yoloCuda_t& yoloCuda);
void releaseCuda(yoloCuda_t& yoloCuda);
void doInference(IExecutionContext& context, cudaStream_t& stream, void** buffers, float* input, float* output, int batchSize) ;

#endif