
#include <stdio.h>
#include "mult_video.h"
#include <iostream>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
#define snprintf _snprintf
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include <libavutil/opt.h>
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
}
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include <libavutil/opt.h>
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#ifdef __cplusplus
};
#endif
#endif

//Output YUV data?
#define ENABLE_YUVFILE 1

//char *filter_descr = "movie=time.mp4[wm];[wm]scale=w=180:h=100[wm];[in][wm]overlay=0:0[out]";

static AVFormatContext *pFormatCtx;
static AVCodecContext *pCodecCtx;
AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;
static int video_stream_index = -1;
const char* main_filename ="testvideo.mp4";



static int open_input_file(const char *filename)
{
    int ret;
    AVCodec *dec;

    if ((ret = avformat_open_input(&pFormatCtx, filename, NULL, NULL)) < 0) {
        printf( "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(pFormatCtx, NULL)) < 0) {
        printf( "Cannot find stream information\n");
        return ret;
    }

    /* select the video stream */
    ret = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        printf( "Cannot find a video stream in the input file\n");
        return ret;
    }
    video_stream_index = ret;

    //pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;
    pCodecCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[video_stream_index]->codecpar);

    /* init the video decoder */
    if ((ret = avcodec_open2(pCodecCtx, dec, NULL)) < 0) {
        printf( "Cannot open video decoder\n");
        return ret;
    }

    return 0;
}

static int open_input_device(const char *url) {
    int ret;
    AVCodec *dec;

    //dev
    avdevice_register_all();
    AVDictionary* options = NULL;
    const char* input_format_name = "video4linux2";
    //const char* url = "/dev/video0";
    //av_dict_set(&options, "video_size", "1280x720", 0);
    //av_dict_set(&options, "input_format", "mjpeg", 0);
    AVInputFormat* input_fmt = av_find_input_format(input_format_name);
    if (input_fmt == NULL) {
        printf("can not find_input_format\n");
        return -1;
    }
    AVFormatContext* format_ctx = avformat_alloc_context();

    if ((ret = avformat_open_input(&pFormatCtx, url, input_fmt, &options)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

//    if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
//        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
//        return ret;
//    }

    if ((ret = avformat_find_stream_info(pFormatCtx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    /* select the video stream */
    ret = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
        return ret;
    }
    video_stream_index = ret;
    pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;
    av_opt_set_int(pCodecCtx, "refcounted_frames", 1, 0);

    /* init the video decoder */
    if ((ret = avcodec_open2(pCodecCtx, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
        return ret;
    }

    return 0;
}

static int init_filters(int filterFlag)
{
    char args[512];
    int ret;
    char filter_descr[512];
    using namespace std;

    // 获取FFmpeg中定义的filter
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    AVRational time_base = pFormatCtx->streams[video_stream_index]->time_base;
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };

    // 创建一个滤波器图filter graph
    filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        printf("avfilter_graph_alloc failed");
        return 0;
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
             time_base.num, time_base.den,
             pCodecCtx->sample_aspect_ratio.num, pCodecCtx->sample_aspect_ratio.den);

    // 创建一个滤波器实例AVFilterContext，并添加到AVFilterGraph中
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
 
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return 0;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");

        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return 0;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");

        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return 0;
    }

    /*
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */

    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

     switch (filterFlag){
        case 1:
            snprintf(filter_descr, sizeof(filter_descr), "movie=testvideo.mp4[wm];[wm]scale=w=%d:h=%d[wm];[in][wm]overlay=%d:%d[out]",pCodecCtx->width/2 ,pCodecCtx->height/2 ,0 ,0);
            break;
        case 2:
            snprintf(filter_descr, sizeof(filter_descr), "movie=testvideo.mp4[wm];[wm]scale=w=%d:h=%d[wm];[in][wm]overlay=%d:%d[out]",pCodecCtx->width/2 ,pCodecCtx->height/2 ,pCodecCtx->width/2 ,0);
            break;
        case 3:
            snprintf(filter_descr, sizeof(filter_descr), "movie=testvideo.mp4[wm];[wm]scale=w=%d:h=%d[wm];[in][wm]overlay=%d:%d[out]",pCodecCtx->width/2 ,pCodecCtx->height/2 ,0 ,pCodecCtx->height/2);
            break;
        case 4:
            snprintf(filter_descr, sizeof(filter_descr), "movie=testvideo.mp4[wm];[wm]scale=w=%d:h=%d[wm];[in][wm]overlay=%d:%d[out]",pCodecCtx->width/2 ,pCodecCtx->height/2 ,pCodecCtx->width/2 ,pCodecCtx->height/2);
            break;
        default:
            break;
    }

    cout << filter_descr << endl;
    /* 将一串通过字符串描述的Graph添加到FilterGraph中。 */
    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_descr,
                                        &inputs, &outputs, NULL)) < 0)
        {
        printf("avfilter_graph_parse_ptr failed");
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return 0;
        }

    /* 检查FilterGraph的配置 */
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        {
        printf("avfilter_graph_parse_ptr failed");
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return 0;
        }

    return ret;
}


int filter_video(int filterFlag)
{
    int ret;
    AVPacket packet;
    AVFrame *pFrame;
    AVFrame *pFrame_out;
    using namespace std;
    //errno_t err;

    //int got_frame = 1;

    /* 视频输入流（主视频） */
    //if ((ret = open_input_file(main_filename)) < 0){
    //    printf("Error open input file\n");
    //    return 0;
     // }
    if ((ret = open_input_device("/dev/video2")) < 0){
        printf("Error open input file\n");
        return 0;
      }
    
    /* 初始化filter */
    if ((ret = init_filters(filterFlag)) < 0){
        printf("Error init filters \n");
        return 0;
      }

#if ENABLE_YUVFILE
    FILE *fp_yuv=fopen("test.yuv","wb+");
#endif

    pFrame=av_frame_alloc();
    pFrame_out=av_frame_alloc();

    /* read all packets */
    while (av_read_frame(pFormatCtx, &packet) >= 0) {


        /* 读取帧数据 */
//        ret = av_read_frame(pFormatCtx, &packet);
//        if (ret< 0)
//            break;

        if (packet.stream_index == video_stream_index) {

            /* 解码一帧视频 */
//            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_frame, &packet);
//            if (ret < 0) {
//                printf( "Error decoding video\n");
//                break;
//            }

            /* decode video frame */
            ret = avcodec_send_packet(pCodecCtx, &packet);
            if (ret < 0) {
                fprintf(stderr, "Error sending a packet for decoding\n");
                exit(1);
            }

            //avcodec_receive_frame(pCodecCtx, pFrame);
            while (ret >= 0) {
                ret = avcodec_receive_frame(pCodecCtx, pFrame);
                //printf("ret = %d \n",ret);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    fprintf(stderr, "Error during decoding\n");
                    exit(1);
                }
                //if (ret >= 0) {
                /* 得到frame的pts */
                pFrame->pts = av_frame_get_best_effort_timestamp(pFrame);

                cout << "pts =  " << pFrame->pts <<endl;
                /* push the decoded frame into the filtergraph */
                if (av_buffersrc_add_frame_flags(buffersrc_ctx, pFrame,AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    printf( "Error while feeding the filtergraph\n");
                    break;
                }

                /* pull filtered  from the filtergraph */
                while (1) {

                    ret = av_buffersink_get_frame(buffersink_ctx, pFrame_out);
                    if (ret < 0)
                        break;

                    printf("Process 1 frame!\n");

                    printf("pFrame_out->height = %d,pFrame_out->width = %d\n",pFrame_out->height,pFrame_out->width);
                    /* 逐帧转存为yuv形式文件 */
                    if (pFrame_out->format==AV_PIX_FMT_YUV420P) {
#if ENABLE_YUVFILE
                        //Y, U, V

                        for(int i=0;i<pFrame_out->height;i++){
                            fwrite(pFrame_out->data[0]+pFrame_out->linesize[0]*i,1,pFrame_out->width,fp_yuv);
                        }

                        for(int i=0;i<pFrame_out->height/2;i++){
                            fwrite(pFrame_out->data[1]+pFrame_out->linesize[1]*i,1,pFrame_out->width/2,fp_yuv);
                        }
                        for(int i=0;i<pFrame_out->height/2;i++){
                            fwrite(pFrame_out->data[2]+pFrame_out->linesize[2]*i,1,pFrame_out->width/2,fp_yuv);
                        }
#endif


                    }
                    av_frame_unref(pFrame_out);
                    //}
                }

            }
            av_frame_unref(pFrame);
        }
        av_frame_unref(pFrame);
        av_packet_unref(&packet);
    }
#if ENABLE_YUVFILE
    fclose(fp_yuv);
#endif


    return 0;
}
