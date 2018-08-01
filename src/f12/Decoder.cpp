#include "Decoder.h"

using namespace std;

namespace DeepGlint
{

    Decoder::Decoder()
    {

    }

    Decoder::~Decoder()
    {

    }

    DecoderErrorCode Decoder::DecoderInit(string rtsp_addr, AVFormatContext *inFtmCtx, AVCodecContext *inCodecCtx, AVDictionary *avDic, time_t &startTime, int&videoIndex)
    {
        AVCodec *inCodec = NULL;
        AVStream *inStream = NULL;

        int ret = 0;
        int i = 0;

        av_register_all();

        ret = avformat_network_init();
        MACRO_CHECK_RETURN(ret);

        ret = av_dict_set(&avDic, option_mean_key, option_mean_value, DEFAULT);
        MACRO_CHECK_RETURN(ret);

        ret = av_dict_set(&avDic, option_delay_key, option_delay_value, DEFAULT);
        MACRO_CHECK_RETURN(ret);

        startTime = time(NULL);
        inFtmCtx->interrupt_callback.callback = TimeOutCallBack;
        inFtmCtx->interrupt_callback.opaque = &startTime;

        ret = avformat_open_input(&inFtmCtx, rtsp_addr.c_str(), DEFAULT, &avDic);
        if(ret != 0)
        {
            return DecodeTimeOut;
        }

        ret = avformat_find_stream_info(inFtmCtx, DEFAULT);
        MACRO_CHECK_RETURN(ret);

        for(i = 0; i < inFtmCtx->nb_streams; i++)
        {
            if(inFtmCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                videoIndex = i;
                break;
            }
        }

        inStream = inFtmCtx->streams[videoIndex];

        inCodec = avcodec_find_decoder(inStream->codecpar->codec_id);

        inCodecCtx = avcodec_alloc_context3(inCodec);

        if(inCodec->capabilities & CODEC_CAP_TRUNCATED)
        {
            inCodecCtx->flags|=CODEC_FLAG_TRUNCATED;
        }

        ret = avcodec_open2(inCodecCtx, inCodec,NULL);

        if(ret != 0)
        {

            avcodec_free_context(&inCodecCtx);
            return DecodeUnknownError;
        }
        return DecodeSucceed;
    }

    DecoderErrorCode Decoder::DecoderProcess(AVFormatContext *inFtmCtx, AVCodecContext *inCodecCtx, AVFrame *pFrame, time_t &startTime, int &videoIndex)
    {

        AVPacket pkt;
        int i, length = 0;
        startTime = time(NULL);

        if(!av_read_frame(inFtmCtx, &pkt))
        {
            if(pkt.stream_index == videoIndex && pkt.flags == AV_PKT_FLAG_KEY)  //video frame
            {
                avcodec_send_packet(inCodecCtx, &pkt);
                avcodec_receive_frame(inCodecCtx, pFrame);
                if (pFrame->key_frame == 1)
                {
                    int picSize = inCodecCtx->height * inCodecCtx->width;
                    int newSize = picSize * 1.5;
                    unsigned char buf[newSize];

                    int height = inCodecCtx->height;
                    int width = inCodecCtx->width;

                    for (i=0; i<height; i++)
                    {
                        memcpy(buf + length, pFrame->data[0] + i * pFrame->linesize[0], width);
                        length += width;
                    }
                    for (i=0; i<height/2; i++)
                    {
                        memcpy(buf + length, pFrame->data[1] + i * pFrame->linesize[1], width/2);
                        length += width/2;
                    }
                    for (i=0; i<height/2; i++)
                    {
                        memcpy(buf + length, pFrame->data[2] + i * pFrame->linesize[2], width/2);
                        length += width/2;
                    }
                    printf("deocde\n");
#ifdef SAVEFILE
                    FILE *pFile=fopen("decoder_saved_yuv", "wb");
                    if(pFile == NULL)
                    {
                        printf("file empty \n");
                        return DecodeSaveFileError;
                    }
                    fwrite(buf, sizeof(char), newSize, pFile);
                    fclose(pFile);
#endif
                }
            }
            av_packet_unref(&pkt);
            startTime = time(NULL);
            return DecodeSucceed;
        }
        return DecodeTimeOut;
    }

    int Decoder::TimeOutCallBack(void *ctx)
    {
        time_t* p = (time_t*)ctx;
        if((time(NULL)-*p) >= MAX_TIMEOUT)
        {
            printf("timeout");
        }
        return (time(NULL)-*p) >= MAX_TIMEOUT ? 1 :0;
    }

}


