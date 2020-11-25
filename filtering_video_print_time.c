#define _XOPEN_SOURCE 600 /* for usleep */

#include <unistd.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
//#include <libavfilter/avfiltergraph.h>
//#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>

//#include <stdlib.h>
#include <time.h>

//缩放滤镜，以下命令将视频缩小一半,iw,ih指的是输入的视频宽和高，
//此外也可以直接使用数字知名缩放的大小，比如:scale=200:100
//const char *filter_descr = "scale=iw/2:ih/2";
/*裁剪滤镜，一下命令将视频的左上角的四分之一裁剪下来*/
//const char *filter_descr = "crop=iw/2:ih/2:0:0";
/*添加字符串水印*/
//const char *filter_descr = "drawtext=fontfile=FreeSans.ttf:fontcolor=blue@0.2:fontsize=90:x=200:y=500:box=1: boxcolor=red@0.2:text='Hello,world'";
const char *filter_descr = "movie=coverr2.mp4[wm];[in][wm]overlay=100:200[out]";

static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;
static int video_stream_index = -1;
static int64_t last_pts = AV_NOPTS_VALUE;

static int open_input_file(const char *filename) {
    int ret;
    AVCodec *dec;

    if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    /* select the video stream */
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
        return ret;
    }
    video_stream_index = ret;
    dec_ctx = fmt_ctx->streams[video_stream_index]->codec;
    av_opt_set_int(dec_ctx, "refcounted_frames", 1, 0);

    /* init the video decoder */
    if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
        return ret;
    }

    return 0;
}

static int init_filters(const char *filters_descr) {
    char args[512];
    int ret = 0;
    AVFilter *buffersrc = avfilter_get_by_name("buffer");
    AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};

    filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
             time_base.num, time_base.den,
             dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
        goto end;
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
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                        &inputs, &outputs, NULL)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto end;

    end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

static int update_filters() {

}

FILE *file_fd;

static void write_frame(const AVFrame *frame) {
    if (frame->format == AV_PIX_FMT_YUV420P) {
        printf("format is yuv420p\n");
    } else {
        printf("format is not yuv420p\n");
    }

    printf("frame widht=%d,frame height=%d\n", \
        frame->width, frame->height);
    fwrite(frame->data[0], 1, frame->width * frame->height, file_fd);
    fwrite(frame->data[1], 1, frame->width / 2 * frame->height / 2, file_fd);
    fwrite(frame->data[2], 1, frame->width / 2 * frame->height / 2, file_fd);
}

int filtering_video(char *filename) {
    clock_t time;
    time = clock();//开始时间

    //std::cout<<"time = "<<1000*double(end-start)/CLOCKS_PER_SEC<<"ms"<<std::endl;
    int ret;
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    int got_frame;
    file_fd = fopen("hello.yuv", "wb+");
    if (!frame || !filt_frame) {
        perror("Could not allocate frame");
        exit(1);
    }

    av_register_all();
    avfilter_register_all();

    if ((ret = open_input_file(filename)) < 0)
        goto end;
    if ((ret = init_filters(filter_descr)) < 0)
        goto end;

    time = clock() - time;   //时间间隔
    double cost_time = 1000 * (double) time / CLOCKS_PER_SEC;
    time = clock();
    printf("init costs time: %f ms\n", cost_time);

    /* read all packets */
    while (1) {
        if ((ret = av_read_frame(fmt_ctx, &packet)) < 0)
            break;
        time = clock() - time;   //时间间隔
        cost_time = 1000 * (double) time / CLOCKS_PER_SEC;
        time = clock();
        printf("read_frame costs time: %f ms\n", cost_time);

        if (packet.stream_index == video_stream_index) {
            got_frame = 0;
            ret = avcodec_decode_video2(dec_ctx, frame, &got_frame, &packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error decoding video\n");
                break;
            }
            time = clock() - time;   //时间间隔
            cost_time = 1000 * (double) time / CLOCKS_PER_SEC;
            time = clock();
            printf("avcodec_decode_video2 costs time: %f ms\n", cost_time);

            if (got_frame) {
                frame->pts = av_frame_get_best_effort_timestamp(frame);
                time = clock() - time;   //时间间隔
                cost_time = 1000 * (double) time / CLOCKS_PER_SEC;
                time = clock();
                printf("av_frame_get_best_effort_timestamp costs time: %f ms\n", cost_time);
                /* push the decoded frame into the filtergraph */
                if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                    break;
                }
                time = clock() - time;   //时间间隔
                cost_time = 1000 * (double) time / CLOCKS_PER_SEC;
                time = clock();
                printf("push the decoded frame into the filtergraph costs time: %f ms\n", cost_time);
                /* pull filtered frames from the filtergraph */
                while (1) {
                    ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0)
                        goto end;
                    time = clock() - time;   //时间间隔
                    cost_time = 1000 * (double) time / CLOCKS_PER_SEC;
                    time = clock();
                    printf("pull filtered frames from the filtergraph costs time: %f ms\n", cost_time);
                    write_frame(filt_frame);
                    av_frame_unref(filt_frame);

                    time = clock() - time;   //时间间隔
                    cost_time = 1000 * (double) time / CLOCKS_PER_SEC;
                    time = clock();
                    printf("write_frame costs time: %f ms\n", cost_time);
                }
                av_frame_unref(frame);
            }
        }
        av_free_packet(&packet);
    }
    end:
    avfilter_graph_free(&filter_graph);
    avcodec_close(dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        exit(1);
    }
    fclose(file_fd);
    exit(0);
}
