#include<stdio.h>

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

const char *in_filename="rtsp://admin:admin@192.168.19.101:554/cam/realmonitor?channel=1&subtype=0";
const char *out_filename="receive.flv";

const char option_mean_key[]="rtsp_transport";
const char option_mean_value[]="tcp";

const char option_delay_key[]="max_delay";
const char option_delay_value[]="5000000";


void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame)
{
    FILE *pFile;
    char szFilename[32];
    int  y;

    // Open file
    sprintf(szFilename, "frame/frame%d.ppm", iFrame);
    pFile=fopen(szFilename, "wb");
    if(pFile==NULL)
        return;

    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for(y=0; y<height; y++) {
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
    }

    fclose(pFile);
}

int main(int argc,char **argv)
{
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL;
    AVFormatContext *ofmt_ctx = NULL;
    AVCodecContext *in_codec_ctx = NULL;
    AVStream *in_stream;
    AVCodec *in_codec;
    AVStream *out_stream;
    AVCodec *out_codec;
    AVCodecContext *out_codec_ctx;


    AVPacket pkt;
    int ret = 0;
    int i = 0;
    int video_index = -1;
    int frame_index = 0;

    AVFrame *pFrame = av_frame_alloc();

    AVFrame *pFrameRGB = av_frame_alloc();

    if(pFrameRGB==NULL)
    {
        return -1;
    }

    av_register_all();

    avformat_network_init();

    AVDictionary *avdic=NULL;
    av_dict_set(&avdic, option_mean_key, option_mean_value, 0);
    av_dict_set(&avdic, option_delay_key, option_delay_value, 0);

    if((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, &avdic)) < 0)
    {
        printf("Could not open input file.\n");
        goto end;
    }

    if((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0)
    {
        printf("Failed to retrieve input stream information\n");
        goto end;
    }

    for(i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        if(ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_index = i;
            break;
        }
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);

    if(!ofmt_ctx)
    {
        printf("Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    ofmt = ofmt_ctx->oformat;

    in_stream = ifmt_ctx->streams[video_index];

    in_codec = avcodec_find_decoder(in_stream->codecpar->codec_id);

    in_codec_ctx = avcodec_alloc_context3(in_codec);

    if(in_codec->capabilities & CODEC_CAP_TRUNCATED)
    {
        in_codec_ctx->flags|=CODEC_FLAG_TRUNCATED;
    }

    if(avcodec_open2(in_codec_ctx, in_codec,NULL) != 0)
    {
        printf("decode errot \n");
        return -1;
    }

    while(1)
    {
        ret = av_read_frame(ifmt_ctx, &pkt);
        if(ret < 0)
            break;

        if(pkt.stream_index == video_index)  //video frame
        {
            printf("Receive %8d video frames from input URL\n", frame_index);

            avcodec_send_packet(in_codec_ctx, &pkt);

            avcodec_receive_frame(in_codec_ctx, pFrame);

            if (pFrame->key_frame == 1)
            {

                int picSize = in_codec_ctx->height * in_codec_ctx->width;
                int newSize = picSize * 1.5;

                unsigned char *buf = new unsigned char[newSize];

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

                FILE *pFile;
                char szFilename[32];

                sprintf(szFilename, "frame%d.ppm", frame_index);
                printf("%s\n", szFilename);
                pFile=fopen(szFilename, "wb");
                if(pFile == NULL)
                {
                    printf("file empty \n");
                    return -1;
                }

                fwrite(buf, 1, newSize, pFile);

                fclose(pFile);

                delete [] buf;

                frame_index++;
            }
        }

    }

    av_write_trailer(ofmt_ctx);

    end:
    av_dict_free(&avdic);
    avformat_close_input(&ifmt_ctx);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameRGB);

    if(ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
    {
        avio_close(ofmt_ctx->pb);
    }
    avformat_free_context(ofmt_ctx);
    if(ret < 0 && ret != AVERROR_EOF)
    {
        printf("Error occured.\n");
        return -1;
    }
    return 0;
}