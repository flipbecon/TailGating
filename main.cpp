#include<stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include<libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

const char *in_filename="rtsp://admin:admin@192.168.19.101:554/cam/realmonitor?channel=1&subtype=0";
const char *out_filename="receive.flv";

const char option_mean_key[]="rtsp_transport";
const char option_mean_value[]="tcp";

const char option_delay_key[]="max_delay";
const char option_delay_value[]="5000000";

int main(int argc,char **argv)
{
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL;
    AVFormatContext *ofmt_ctx = NULL;
    AVPacket pkt;
    int ret = 0;
    int i = 0;
    int video_index = -1;
    int frame_index = 0;

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
    for(i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        AVStream *in_stream = ifmt_ctx->streams[i];

        AVCodec *in_codec = avcodec_find_decoder(in_stream->codecpar->codec_id);

        AVCodecContext *in_codec_ctx = avcodec_alloc_context3(in_codec);

        avcodec_parameters_to_context(in_codec_ctx, in_stream->codecpar);

        AVStream *out_stream = avformat_new_stream(ofmt_ctx,in_codec_ctx->codec);

        if(!out_stream)
        {
            avcodec_free_context(&in_codec_ctx);
            printf("Failed allocating output stream.\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);

        AVCodec *out_codec = avcodec_find_decoder(out_stream->codecpar->codec_id);

        AVCodecContext *out_codec_ctx = avcodec_alloc_context3(out_codec);

        ret = avcodec_parameters_to_context(out_codec_ctx, out_stream->codecpar);

        if(ret < 0)
        {
            avcodec_free_context(&out_codec_ctx);
            printf("Failed to copy context from input to output stream codec context\n");
            goto end;
        }
        out_codec_ctx->codec_tag = 0;

        if(ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        {
            out_codec_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
        }
        avcodec_free_context(&in_codec_ctx);
        avcodec_free_context(&out_codec_ctx);
    }

    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    if(!(ofmt->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if(ret<0)
        {
            printf("Could not open output URL '%s'", out_filename);
            goto end;
        }
    }

    ret = avformat_write_header(ofmt_ctx, NULL);
    if(ret < 0)
    {
        printf("Error occured when opening output URL\n");
        goto end;
    }

    while(1)
    {
        AVStream *in_stream;
        AVStream *out_stream;

        ret = av_read_frame(ifmt_ctx, &pkt);
        if(ret < 0)
            break;

        in_stream = ifmt_ctx->streams[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];

        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));

        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;

        if(pkt.stream_index == video_index)
        {
            printf("Receive %8d video frames from input URL\n", frame_index);
            frame_index++;
        }

        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if(ret < 0)
        {
            if(ret==-22)
            {
                continue;
            }
            else
            {
                printf("Error muxing packet.error code %d\n" , ret);
                break;
            }
        }
        av_packet_unref(&pkt);
    }

    av_write_trailer(ofmt_ctx);

    end:
    av_dict_free(&avdic);
    avformat_close_input(&ifmt_ctx);

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