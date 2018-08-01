#include <string>
#include <iostream>
#include <thread>
#include <future>
#include <vector>
#include <tbb/pipeline.h>
#include <tbb/concurrent_queue.h>

#include "Decoder.h"

using namespace std;

#define MAX_THREAD 20

int RtspDecodePipeline(string& rtsp_src)
{

    int ret= 0;

    int videoIndex = -1;
    AVDictionary *avDic = NULL;
    AVCodecContext inCodecCtx;
    time_t timeOut;

    AVFormatContext *inFtmCtx = avformat_alloc_context();

    ret = DeepGlint::Decoder::DecoderInit(rtsp_src, inFtmCtx, &inCodecCtx, avDic, timeOut, videoIndex);

    if(ret != DeepGlint::DecodeSucceed)
    {
        avformat_free_context(inFtmCtx);
        return 0;
    }

    AVFrame *pFrame = av_frame_alloc();
    if(pFrame == NULL)
    {
        avformat_free_context(inFtmCtx);
        return 0;
    }
    while(1)
    {
        ret = DeepGlint::Decoder::DecoderProcess(inFtmCtx, &inCodecCtx, pFrame, timeOut, videoIndex);
        //std::cout<< ret << std::endl;
        if(ret != DeepGlint::DecodeSucceed)
        {
            avformat_free_context(inFtmCtx);
            av_frame_free(&pFrame);
            return 0;
        }
    }
}


int main(int argc,char **argv)
{
    std::string file_name_pool[MAX_THREAD];

    for(int i = 0; i < MAX_THREAD; ++i)
    {
        file_name_pool[i] = "rtsp://192.168.2.123/live/t1030";
    }

    std::thread thread_pool[MAX_THREAD];
    for(int i =0; i< MAX_THREAD; ++i)
    {
        thread_pool[i] = thread(RtspDecodePipeline, std::ref(file_name_pool[i]));
    }
    for(auto &t : thread_pool)
    {
        t.join();
    }

    std::cout << "all threads joined" << std::endl;

    return 0;
}