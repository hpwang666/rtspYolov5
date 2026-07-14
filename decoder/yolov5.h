
#ifndef _YOLOV5_H
#define _YOLOV5_H


#include "yololayer.h"
#include "logging.h"



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
    uint8_t *rgb_out_buffer;
    uint8_t *rgb_in_buffer;
    uint8_t* img_host ;
    uint8_t* img_device ;


    
}yoloCuda_t;


void initCuda(yoloCuda_t& yoloCuda);
void releaseCuda(yoloCuda_t& yoloCuda);
void doInference(IExecutionContext& context, cudaStream_t& stream, void **buffers, float* output, int batchSize) ;

#endif