#include<stdio.h>
#include<unistd.h>
#include <string>
#include <iostream>
#include <thread>
#include <future>
#include <vector>
#include <tbb/pipeline.h>
#include <tbb/concurrent_queue.h>

using namespace std;

#ifdef __cplusplus
extern "C"
{
#endif

#include<libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

#define MAX_THREAD 30


#define MACRO_CHECK_NULL(var) \
        if(var == NULL) \
        {                   \
            printf("%s, %d, %s",__FILE__, __LINE__, __FUNCTION__); \
            return -1; \
        }


#define MACRO_CHECK_RETURN(var) \
        if(var < 0) \
        {                   \
            printf("%s, %d, %s",__FILE__, __LINE__, __FUNCTION__); \
            return -1; \
        }

const char option_mean_key[]="rtsp_transport";
const char option_mean_value[]="tcp";

const char option_delay_key[]="max_delay";
const char option_delay_value[]="5000000";

int TimeOutCallBack(void* ctx)
{
    auto p = (time_t*)ctx;
    return (time(NULL)-*p) >= 5 ? 1 :0;
}

int RtspDecodePipeline(string& rtsp_src)
{
    AVPacket pkt;
    AVCodec *in_codec = NULL;
    AVStream *in_stream = NULL;
    AVDictionary *avdic = NULL;
    AVCodecContext *in_codec_ctx = NULL;

    int ret = 0;
    int i = 0;
    int video_index = -1;
    int frame_index = 0;
    time_t StartTime;

    printf("thread id = %x running \n", this_thread::get_id());

    AVFrame *pFrame = av_frame_alloc();
    MACRO_CHECK_NULL(pFrame);

    AVFrame *pFrameRGB = av_frame_alloc();
    MACRO_CHECK_NULL(pFrameRGB);

    AVFormatContext *ifmt_ctx = avformat_alloc_context();
    MACRO_CHECK_NULL(ifmt_ctx);

    av_register_all();

    ret = avformat_network_init();
    MACRO_CHECK_RETURN(ret);

    ret = av_dict_set(&avdic, option_mean_key, option_mean_value, 0);
    MACRO_CHECK_RETURN(ret);

    ret = av_dict_set(&avdic, option_delay_key, option_delay_value, 0);
    MACRO_CHECK_RETURN(ret);

    StartTime = time(NULL);
    ifmt_ctx->interrupt_callback.callback = TimeOutCallBack;
    ifmt_ctx->interrupt_callback.opaque = &StartTime;

    ret = avformat_open_input(&ifmt_ctx, rtsp_src.c_str(), 0, &avdic);
    MACRO_CHECK_RETURN(ret);

    ret = avformat_find_stream_info(ifmt_ctx, 0);
    MACRO_CHECK_RETURN(ret);

    for(i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        if(ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_index = i;
            break;
        }
    }

    in_stream = ifmt_ctx->streams[video_index];

    in_codec = avcodec_find_decoder(in_stream->codecpar->codec_id);

    in_codec_ctx = avcodec_alloc_context3(in_codec);

    if(in_codec->capabilities & CODEC_CAP_TRUNCATED)
    {
        in_codec_ctx->flags|=CODEC_FLAG_TRUNCATED;
    }

    ret = avcodec_open2(in_codec_ctx, in_codec,NULL);
    MACRO_CHECK_RETURN(ret);

    StartTime = time(NULL);

    while(!av_read_frame(ifmt_ctx, &pkt))
    {
        if(pkt.stream_index == video_index && pkt.flags == AV_PKT_FLAG_KEY)  //video frame
        {
            avcodec_send_packet(in_codec_ctx, &pkt);
            avcodec_receive_frame(in_codec_ctx, pFrame);

            if (pFrame->key_frame == 1)
            {
                int picSize = in_codec_ctx->height * in_codec_ctx->width;
                int newSize = picSize * 1.5;

                unsigned char buf[newSize];

                int height = in_codec_ctx->height;
                int width = in_codec_ctx->width;

                int a=0;
                for (i=0; i<height; i++)
                {
                    memcpy(buf+a,pFrame->data[0] + i * pFrame->linesize[0], width);
                    a+=width;
                }
                for (i=0; i<height/2; i++)
                {
                    memcpy(buf+a,pFrame->data[1] + i * pFrame->linesize[1], width/2);
                    a+=width/2;
                }
                for (i=0; i<height/2; i++)
                {
                    memcpy(buf+a,pFrame->data[2] + i * pFrame->linesize[2], width/2);
                    a+=width/2;
                }
                printf("%d \n", frame_index);
/*
                FILE *pFile;
                char szFilename[32];

                sprintf(szFilename, "frame%d.yuv", frame_index);
                printf("%s\n", szFilename);
                pFile=fopen(szFilename, "wb");
                if(pFile == NULL)
                {
                    printf("file empty \n");
                    goto end;
                }

                fwrite(buf, 1, newSize, pFile);

                fclose(pFile);
*/
                frame_index++;
            }
        }
        av_packet_unref(&pkt);
        StartTime = time(NULL);
    }
    printf("exit thread \n");
    av_dict_free(&avdic);
    avformat_close_input(&ifmt_ctx);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameRGB);
    avformat_free_context(ifmt_ctx);

    return 0;
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