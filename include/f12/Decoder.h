#ifndef __DECODER_H
#define __DECODER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

#include <string>

#define DEFAULT     0
#define MAX_TIMEOUT 5

#define MACRO_CHECK_NULL(var) \
        if(var == NULL) \
        {                   \
            printf("%s, %d, %s",__FILE__, __LINE__, __FUNCTION__); \
            return DeepGlint::DecoderErrorCode::DecodeUnknownError; \
        }


#define MACRO_CHECK_RETURN(var) \
        if(var < 0) \
        {                   \
            printf("%s, %d, %s",__FILE__, __LINE__, __FUNCTION__); \
            return DeepGlint::DecoderErrorCode::DecodeUnknownError; \
        }

const char option_mean_key[] = "rtsp_transport";
const char option_mean_value[] = "tcp";

const char option_delay_key[] = "max_delay";
const char option_delay_value[] = "5000000";

namespace DeepGlint {

    enum DecoderErrorCode
    {
        DecodeSucceed= 0,
        DecodeTimeOut,
        DecodeSaveFileError,
        DecodeUnknownError
    };

    class Decoder {

    public:

        Decoder();

        ~Decoder();

    public:

        static DeepGlint::DecoderErrorCode DecoderInit(std::string rtsp_addr, AVFormatContext *inFtmCtx, AVCodecContext *inCodecCtx, AVDictionary *avDic, time_t &startTime, int&videoIndex);

        static DeepGlint::DecoderErrorCode DecoderProcess(AVFormatContext *inFtmCtx, AVCodecContext *inCodecCtx, AVFrame *pFrame, time_t &startTime, int &videoIndex);

        static int TimeOutCallBack(void *ctx);
    };
}
#endif