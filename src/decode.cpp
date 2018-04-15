



#include "decode.h"
#include "yuv2bgr.h"
#include "opencv2/gpu/gpu.hpp"
#include <cuda_runtime.h>
#include <cuda_profiler_api.h>
#include <curand.h>
#include <dynlink_cuviddec.h>

#define STREAM_DURATION   5.0  
#define STREAM_FRAME_RATE 25 /* 25 images/s */  
#define STREAM_NB_FRAMES  ((int)(STREAM_DURATION * STREAM_FRAME_RATE))  
#define STREAM_PIX_FMT AV_PIX_FMT_YUV420P /* default pix_fmt */  

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

static volatile int DISTURBE_SIGNALS   = 0;
static int decode_interrupt_cb(void *ctx) {
        return DISTURBE_SIGNALS > 0;
}
static const AVIOInterruptCB int_cb = { decode_interrupt_cb, NULL };

int ffmpeg_decode(AVFormatContext *ic,
                const int type,
                const int stream_id,
                int (*cpu_cb)(const int type, cv::Mat&),
                int (*gpu_cb)(const int type, cv::gpu::GpuMat &),
                bool use_hw_decode,
                bool only_key_frame);

int ffmpeg_global_init() {
        avcodec_register_all();
        avdevice_register_all();
        avfilter_register_all();
        av_register_all();
        avformat_network_init();
        av_log_set_level(AV_LOG_INFO);
        printf("finish init ffmpeg !\n");
        return 0;
}



int ffmpeg_video_decode(const std::string &addr,
                const int type,
                int (*cpu_cb)(const int type, cv::Mat&),
                int (*gpu_cb)(const int type, cv::gpu::GpuMat &),
                bool use_hw_decode,
                bool only_key_frame){
        //av_log(NULL, AV_LOG_INFO, "stream path:%s, \nuse_hw_decode:%s, \nonly_key_frame:%s,\n", addr.c_str(), 
        //              use_hw_decode ? "true" : "false",
        //              only_key_frame ? "true" : "false");

        ffmpeg_global_init();
        AVFormatContext *ic;
        ic = avformat_alloc_context();
        if (!ic) {
                printf("error !\n");return -1;
        }
        printf("1\n");
        AVFormatContext *pFormatCtx;
    int             i, videoindex;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame *pFrame,*pFrameYUV;
    uint8_t *out_buffer;
    AVPacket *packet;
    int y_size;
    int ret, got_picture;
    struct SwsContext *img_convert_ctx;
pFormatCtx = avformat_alloc_context();//�~H~]��~K�~L~V��~@个AVFormatContext

    if(avformat_open_input(&pFormatCtx,addr.c_str(),NULL,NULL)!=0){//�~I~S��~@��~S�~E��~Z~D��~F��~Q�~V~G件
        printf("Couldn't open input stream.\n");
        return -1;
    }
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){//�~N��~O~V��~F��~Q�~V~G件信�~A�
        printf("Couldn't find stream information.\n");
        return -1;
    }
    videoindex=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            videoindex=i;
            break;
        }

    if(videoindex==-1){
        printf("Didn't find a video stream.\n");
        return -1;
    }
pCodecCtx=pFormatCtx->streams[videoindex]->codec;

    int video_stream_idx = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream_idx >= 0) {
    AVStream *video_stream = pFormatCtx->streams[video_stream_idx];
    //pCodecCtx->codec_id = cudaVideoCodec_H264;//cudaVideoCodec_H264;
    printf("input format: %s\n", pFormatCtx->iformat->name);
    printf("video nb_frames: %lld\n", video_stream->nb_frames);
    printf("video codec_name: %s\n", avcodec_get_name(video_stream->codec->codec_id));
    printf("video width x height: %d x %d\n", video_stream->codec->width, video_stream->codec->height);
    printf("video pix_fmt: %d\n", video_stream->codec->pix_fmt);
    printf("video bitrate %lld kb/s\n", (int64_t) video_stream->codec->bit_rate / 1000);
    //printf("video avg_frame_rate: %d fps\n", video_stream->avg_frame_rate.num/video_stream->avg_frame);
    std::cout<<"pCodecCtx->codec_id = "<< pCodecCtx->codec_id<<std::endl;
    }
    if(use_hw_decode == true){
    pCodec=avcodec_find_decoder_by_name("h264_cuvid");//�~_��~I�解�| ~A�~Y�
    }
    else if(use_hw_decode == false){
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);//�~_��~I�解�| ~A�~Y�
    }

    printf("--------------- File Information ----------------\n");
    if(pCodec==NULL){
        printf("Codec not found.\n");
        return -1;
    }
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){//�~I~S��~@解�| ~A�~Y�
        printf("Could not open codec.\n");
        return -1;
    }

    pFrame=av_frame_alloc();
    pFrameYUV=av_frame_alloc();

    out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

    packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    //Output Info-----------------------------
    //printf("--------------- File Information ----------------\n");

    av_dump_format(pFormatCtx,0,addr.c_str(),0);
    printf("-------------------------------------------------\n");

    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
        pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
     std::cout << "decoding time:       " << std::setw(10) << benchmark([&]() {
    while(av_read_frame(pFormatCtx, packet)>=0){//读�~O~V��~@帧�~N~K缩�~U��~M�
        //std::cout << "decoding time:       " << std::setw(10) << benchmark([&]() {
      if(packet->stream_index==videoindex){
            //1. do h264 data write !!!
            //fwrite(packet->data,1,packet->size,fp_h264); //�~J~JH264�~U��~M��~F~Y�~E�fp_h264�~V~G件
            //2. do decode !!!
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);//解�| ~A��~@帧�~N~K缩�~U��~M�
            if(ret < 0){
                printf("Decode Error.\n");
                return -1;
            }
            //3. do frame resize !!!
            /*if(got_picture){
                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
                    pFrameYUV->data, pFrameYUV->linesize);
              4. write to YUV formate !!!
                y_size=pCodecCtx->width*pCodecCtx->height;  
                fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y 
                fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
                fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
                printf("Succeed to decode 1 frame!\n");

            }*/
           }
          //}) << " ms" << std::endl;


        //Unquote and release packet object, or will lead memory leak
        av_packet_unref(packet);
        av_free_packet(packet);

    }
    }) << " ms" << std::endl;
    printf("finish decoding !\n");
}
