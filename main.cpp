#include <stdio.h>
#include <stdlib.h>
#include "decode.h"
#include <pthread.h> 
#include "opencv2/gpu/gpu.hpp" 
#define TEST
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <algorithm>
using namespace std;

template <typename F>
double benchmark(const F &fun, int n = 1)
{
    double duration = 0.0;
    std::vector<double> timings;
    for (int i = 0; i < n; i++)
    {
        auto start = std::chrono::high_resolution_clock::now();
        fun();
        auto end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        timings.push_back(duration);
    }

    std::sort(timings.begin(), timings.end());
    return timings[timings.size() / 2] / 1000;
}
static int NUM=1;

struct thread_attr
{
   char * video_src_name;
   char * video_src_type;
   bool  coder_device_type = 0;//default on gpu
};

int cpu_callback(const int flag, cv::Mat &image) {
#ifdef TEST
	/*printf("get cpu image, image.width:%d, image.height:%d\n", image.cols, image.rows);
	char filename[100] = {0};
	if (flag == 1) {
		snprintf(filename, 100, "./image/1_%d.jpg", NUM ++);	
	} else {
		snprintf(filename, 100, "./image/2_%d.jpg", NUM ++);	
	}
	printf("filename:%s\n", filename);
	cv::imwrite(filename, image);*/
#endif
	return 0;
}
int gpu_callback(const int flag, cv::gpu::GpuMat &image) {
#ifdef TEST
	/*printf("get gpu image, image.width:%d, image.height:%d\n", image.cols, image.rows);
	cv::Mat mat(image.cols, image.rows, CV_8UC3);
	image.download(mat);
	char filename[100] = {0};

	if (flag == 1) {
		snprintf(filename, 100, "./image/1_%d.jpg", NUM ++);	
	} else {
		snprintf(filename, 100, "./image/2_%d.jpg", NUM ++);	
	}
	printf("filename:%s\n", filename);
	cv::imwrite(filename, mat);
	//cv::imshow("press ESC to exit", mat);
	//cv::waitKey(0);*/
#endif
	return 0;
}

void *videoHandle1(void * arg) {
        
        struct thread_attr * attr_temp = (struct thread_attr *)arg;
        
        std::string video_addr;
        if(attr_temp->video_src_type == "RTSP"){
	   video_addr = "rtsp://admin:stkj1234@10.0.3.144:554";
           //continue;
        }
        if(attr_temp->video_src_type == "USB"){
           video_addr = attr_temp->video_src_name;
           //continue;
        }
        if(attr_temp->video_src_type == "FILE"){
           video_addr = attr_temp->video_src_name;
           //continue;
        }
        else{
           printf("unkown video source type ! \n");
           //exit(1);
        }

	printf("open video:%s\n", video_addr.c_str());
	
	int flag = 1;	
	bool use_hw_decode = true; //使用硬解码
	bool only_key_frame = false; //是否只使用关键帧
	ffmpeg_video_decode(video_addr, flag, cpu_callback, gpu_callback, use_hw_decode, only_key_frame);
}

void *videoHandle2(void *arg) {

        struct thread_attr * attr_temp;
        attr_temp = (struct thread_attr *)arg;
        //printf("attr->video_src_type:%s \n",attr_temp->video_src_type);
        std::string video_addr;
        if(attr_temp->video_src_type == "RTSP"){
	   video_addr = "rtsp://admin:12345@10.0.3.137:554";
	   //printf("open video:%s\n", video_addr.c_str());
           //continue;
        }
	else if(attr_temp->video_src_type == "USB"){
           video_addr = attr_temp->video_src_name;
           //continue;
        }
        else if(attr_temp->video_src_type == "FILE"){
           video_addr = attr_temp->video_src_name;
           //continue;
        }
        else{
           printf("unkown video source type ! \n");
           //exit(1);
        }

        printf("open video:%s\n", video_addr.c_str());

	bool use_hw_decode = false; //使用硬解码
	bool only_key_frame = false; //是否只使用关键帧
	int flag = 2;
	ffmpeg_video_decode(video_addr, flag, cpu_callback, gpu_callback, use_hw_decode, only_key_frame);
}

int main() {
	//ffmpeg初始化
	ffmpeg_global_init();
	
	//thread mode
	pthread_t t1,t2;

        struct thread_attr * attr1;
        attr1 = (struct thread_attr *)malloc(sizeof(struct thread_attr)); 
        attr1->video_src_name = "../data/720p.mp4";
        attr1->video_src_type = "FILE";
        attr1->coder_device_type = 0;
        struct thread_attr * attr2;
        attr2 = (struct thread_attr *)malloc(sizeof(struct thread_attr));
        attr2->video_src_name = "../data/1080p.mp4";
        attr2->video_src_type = "FILE";
        attr2->coder_device_type = 1;
       
        for(int k=0; k<1;k++)
	{
          pthread_create(&t1, NULL, videoHandle1, (void *)attr1/*NULL*/);
	  pthread_create(&t2, NULL, videoHandle2, (void *)attr2/*NULL*/);
        }
        getchar();
       
	return 0;
}
