#include <iostream>
#include <fstream> // 添加此行

#include "cuda_utils.h"
#include <chrono>

// #include "common.hpp"
// #include "utils.h"
// #include "calibrator.h"
 
#include "yolov5.h"


using namespace std;
 
 const char* my_classes[] = { "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
         "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
         "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
         "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard","surfboard",
         "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
         "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
         "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
         "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
         "hair drier", "toothbrush" };

 
// stuff we know about the network and the input/output blobs
static const int INPUT_H = Yolo::INPUT_H;
static const int INPUT_W = Yolo::INPUT_W;
static const int CLASS_NUM = Yolo::CLASS_NUM;
static const int OUTPUT_SIZE = Yolo::MAX_OUTPUT_BBOX_COUNT * sizeof(Yolo::Detection) / sizeof(float) + 1;  // we assume the yololayer outputs no more than MAX_OUTPUT_BBOX_COUNT boxes that conf >= 0.1
static const char* INPUT_BLOB_NAME = "data";
static const char* OUTPUT_BLOB_NAME = "prob";
static Logger gLogger;

 
void doInference(IExecutionContext& context, cudaStream_t& stream, void** buffers, float* input, float* output, int batchSize) {
    // DMA input batch data to device, infer on the batch asynchronously, and DMA output back to host
    CUDA_CHECK(cudaMemcpyAsync(buffers[0], input, batchSize * 3 * INPUT_H * INPUT_W * sizeof(float), cudaMemcpyHostToDevice, stream));
    context.enqueue(batchSize, buffers, stream, nullptr);
    CUDA_CHECK(cudaMemcpyAsync(output, buffers[1], batchSize * OUTPUT_SIZE * sizeof(float), cudaMemcpyDeviceToHost, stream));
    cudaStreamSynchronize(stream);
}
 
void initCuda(yoloCuda_t& yoloCuda)
{
   
  

    yoloCuda.prob=(float*)calloc(OUTPUT_SIZE*BATCH_SIZE,sizeof(float));
    
    
    std::ifstream file("./yolov5n.engine", std::ios::binary);
    if (!file.good()) {
        std::cerr << " read engin file  error! " << std::endl;
        return ;
    }

    char* trtModelStream{ nullptr };
    size_t size = 0;
    file.seekg(0, file.end);
    size = file.tellg();
    file.seekg(0, file.beg);
    trtModelStream = new char[size];
    assert(trtModelStream);
    file.read(trtModelStream, size);
    file.close();

   
    yoloCuda.runtime = createInferRuntime(gLogger);
    printf("read engine file ok \r\n");
    assert(yoloCuda.runtime != nullptr);
    yoloCuda.engine = yoloCuda.runtime->deserializeCudaEngine(trtModelStream, size);
    assert(yoloCuda.engine != nullptr);
    yoloCuda.context = yoloCuda.engine->createExecutionContext();
    assert(yoloCuda.context != nullptr);
    delete[] trtModelStream;
    assert(yoloCuda.engine->getNbBindings() == 2);
 

    // In order to bind the buffers, we need to know the names of the input and output tensors.
    // Note that indices are guaranteed to be less than IEngine::getNbBindings()
    yoloCuda.inputIndex = yoloCuda.engine->getBindingIndex(INPUT_BLOB_NAME);
    yoloCuda.outputIndex = yoloCuda.engine->getBindingIndex(OUTPUT_BLOB_NAME);
    assert(yoloCuda.inputIndex == 0);
    assert(yoloCuda.outputIndex == 1);
    // Create GPU buffers on device
    CUDA_CHECK(cudaMalloc(&yoloCuda.buffers[yoloCuda.inputIndex], BATCH_SIZE * 3 * INPUT_H * INPUT_W * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&yoloCuda.buffers[yoloCuda.outputIndex], BATCH_SIZE * OUTPUT_SIZE * sizeof(float)));


    cudaDeviceProp devProp;
    cudaGetDeviceProperties (&devProp, 0);
    if (!devProp.managedMemory) {
        printf ("CUDA device does not support managed memory.\n");
    }
    else printf("CUDA device supports managed memory.\n");
    cudaMallocManaged ((void **)&yoloCuda.rgb_out_buffer, 1920*1080*3, cudaMemAttachGlobal);
    //CUDA_CHECK( cudaMalloc((void **)&yoloCuda.rgb_out_buffer, 1920*1080*3));
    cudaDeviceSynchronize ();
    unsigned int flags;
    if(cudaHostGetFlags(&flags, yoloCuda.rgb_out_buffer) == cudaSuccess){
         printf("flags :%d\r\n",flags);//&& (flags & cudaHostAllocMapped);
    } 



    // Create stream
   
    CUDA_CHECK(cudaStreamCreate(&yoloCuda.stream));


  
}

void releaseCuda(yoloCuda_t& yoloCuda)
{
    CUDA_CHECK(cudaFree (yoloCuda.rgb_out_buffer));
    free(yoloCuda.prob);
    cudaStreamDestroy(yoloCuda.stream);
    CUDA_CHECK(cudaFree(yoloCuda.buffers[yoloCuda.inputIndex]));
    CUDA_CHECK(cudaFree(yoloCuda.buffers[yoloCuda.outputIndex]));
    // Destroy the engine
    yoloCuda.context->destroy();
    yoloCuda.engine->destroy();
    yoloCuda.runtime->destroy();
}
 
#if 0
int main(int argc, char** argv) {
    cudaSetDevice(DEVICE);
 
    //std::string wts_name = "";
    std::string engine_name = "";
    //float gd = 0.0f, gw = 0.0f;
    //std::string img_dir;
 
    if (!parse_args(argc, argv, engine_name)) {
        std::cerr << "arguments not right!" << std::endl;
        std::cerr << "./yolov5 -v [.engine] // run inference with camera" << std::endl;
        return -1;
    }
 
    std::ifstream file(engine_name, std::ios::binary);
    if (!file.good()) {
        std::cerr << " read " << engine_name << " error! " << std::endl;
        return -1;
    }
    char* trtModelStream{ nullptr };
    size_t size = 0;
    file.seekg(0, file.end);
    size = file.tellg();
    file.seekg(0, file.beg);
    trtModelStream = new char[size];
    assert(trtModelStream);
    file.read(trtModelStream, size);
    file.close();
 
 
    // prepare input data ---------------------------
    static float data[BATCH_SIZE * 3 * INPUT_H * INPUT_W];
    //for (int i = 0; i < 3 * INPUT_H * INPUT_W; i++)
    //    data[i] = 1.0;
    static float prob[BATCH_SIZE * OUTPUT_SIZE];
    IRuntime* runtime = createInferRuntime(gLogger);
    assert(runtime != nullptr);
    ICudaEngine* engine = runtime->deserializeCudaEngine(trtModelStream, size);
    assert(engine != nullptr);
    IExecutionContext* context = engine->createExecutionContext();
    assert(context != nullptr);
    delete[] trtModelStream;
    assert(engine->getNbBindings() == 2);
    void* buffers[2];
    // In order to bind the buffers, we need to know the names of the input and output tensors.
    // Note that indices are guaranteed to be less than IEngine::getNbBindings()
    const int inputIndex = engine->getBindingIndex(INPUT_BLOB_NAME);
    const int outputIndex = engine->getBindingIndex(OUTPUT_BLOB_NAME);
    assert(inputIndex == 0);
    assert(outputIndex == 1);
    // Create GPU buffers on device
    CUDA_CHECK(cudaMalloc(&buffers[inputIndex], BATCH_SIZE * 3 * INPUT_H * INPUT_W * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&buffers[outputIndex], BATCH_SIZE * OUTPUT_SIZE * sizeof(float)));
    // Create stream
    cudaStream_t stream;
    CUDA_CHECK(cudaStreamCreate(&stream));
 
 
    cv::VideoCapture capture("/home/whp/tensorrtx-yolov5-v6.0/yolov5/samples/v1080_1_60.264");	//修改为自己要检测的视频或者图片，注意要写全路径，如果调用摄像头，则括号内的参数设为0，注意引号要去掉。
    //cv::VideoCapture capture("rtsp://admin:@Fhjt0717@192.168.1.44:554/h264/ch1/main/av_stream");
    //cv::VideoCapture capture("../overpass.mp4");
    //int fourcc = cv::VideoWriter::fourcc('M','J','P','G');
    //capture.set(cv::CAP_PROP_FOURCC, fourcc);
    if (!capture.isOpened()) {
        std::cout << "Error opening video stream or file" << std::endl;
        return -1;
    }
 
    int key;
    int fcount = 0;
    while (1)
    {
        cv::Mat frame;
        capture >> frame;
        if (frame.empty())
        {
            std::cout << "Fail to read image from camera!" << std::endl;
            break;
        }
#if 1
        fcount++;
        //if (fcount < BATCH_SIZE && f + 1 != (int)file_names.size()) continue;
        for (int b = 0; b < fcount; b++) {
            //cv::Mat img = cv::imread(img_dir + "/" + file_names[f - fcount + 1 + b]);
            cv::Mat img = frame;
            if (img.empty()) continue;
            cv::Mat pr_img = preprocess_img(img, INPUT_W, INPUT_H); // letterbox BGR to RGB
            int i = 0;
            for (int row = 0; row < INPUT_H; ++row) {
                uchar* uc_pixel = pr_img.data + row * pr_img.step;
                for (int col = 0; col < INPUT_W; ++col) {
                    data[b * 3 * INPUT_H * INPUT_W + i] = (float)uc_pixel[2] / 255.0;
                    data[b * 3 * INPUT_H * INPUT_W + i + INPUT_H * INPUT_W] = (float)uc_pixel[1] / 255.0;
                    data[b * 3 * INPUT_H * INPUT_W + i + 2 * INPUT_H * INPUT_W] = (float)uc_pixel[0] / 255.0;
                    uc_pixel += 3;
                    ++i;
                }
            }
        }
 
        // Run inference
        auto start = std::chrono::system_clock::now();
        doInference(*context, stream, buffers, data, prob, BATCH_SIZE);
        auto end = std::chrono::system_clock::now();
        //std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
        int fps = 1000.0 / std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::vector<std::vector<Yolo::Detection>> batch_res(fcount);
        for (int b = 0; b < fcount; b++) {
            auto& res = batch_res[b];
            nms(res, &prob[b * OUTPUT_SIZE], CONF_THRESH, NMS_THRESH);
        }
        for (int b = 0; b < fcount; b++) {
            auto& res = batch_res[b];
            //std::cout << res.size() << std::endl;
            //cv::Mat img = cv::imread(img_dir + "/" + file_names[f - fcount + 1 + b]);
            for (size_t j = 0; j < res.size(); j++) {
                cv::Rect r = get_rect(frame, res[j].bbox);
                cv::rectangle(frame, r, cv::Scalar(0x27, 0xC1, 0x36), 2);
                std::string label = my_classes[(int)res[j].class_id];
                cv::putText(frame, label, cv::Point(r.x, r.y - 1), cv::FONT_HERSHEY_PLAIN, 1.2, cv::Scalar(0xFF, 0xFF, 0xFF), 2);
                std::string jetson_fps = "Jetson Nano FPS: " + std::to_string(fps);
                cv::putText(frame, jetson_fps, cv::Point(11, 80), cv::FONT_HERSHEY_PLAIN, 3, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
            }
            //cv::imwrite("_" + file_names[f - fcount + 1 + b], img);
        }
#endif
        cv::imshow("yolov5", frame);
	 key = cv::waitKey(1);
        if (key == 'q') {
            break;
        }
        fcount = 0;
    }
 
    capture.release();
    // Release stream and buffers
    cudaStreamDestroy(stream);
    CUDA_CHECK(cudaFree(buffers[inputIndex]));
    CUDA_CHECK(cudaFree(buffers[outputIndex]));
    // Destroy the engine
    context->destroy();
    engine->destroy();
    runtime->destroy();
 
    return 0;
}

#endif