#include "filter_video.h"

#include <time.h>

//缩放滤镜，以下命令将视频缩小一半,iw,ih指的是输入的视频宽和高，
//此外也可以直接使用数字知名缩放的大小，比如:scale=200:100
//const char *filter_descr = "scale=iw/2:ih/2";
//裁剪滤镜，一下命令将视频的左上角的四分之一裁剪下来
//const char *filter_descr = "crop=iw/2:ih/2:0:0";
//添加字符串水印
//const char *filter_descr = "drawtext=fontfile=FreeSans.ttf:fontcolor=blue@0.2:fontsize=90:x=200:y=500:
//                            box=1: boxcolor=red@0.2:text='Hello,world'";
//画中画
//const char *filter_descr = "movie=coverr2.mp4[wm];[in][wm]overlay=x=400:y=500[out]";

int FilterVideo::init_filter(const char *filters_descr, char *args) {
    int ret = 0;
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};

    this->filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !this->filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    ret = avfilter_graph_create_filter(&this->buffersrc_ctx, buffersrc, "in",
                                       args, NULL, this->filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&this->buffersink_ctx, buffersink, "out",
                                       NULL, NULL, this->filter_graph);
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
    outputs->filter_ctx = this->buffersrc_ctx;
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

    if ((ret = avfilter_graph_parse_ptr(this->filter_graph, filters_descr,
                                        &inputs, &outputs, NULL)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(this->filter_graph, NULL)) < 0)
        goto end;

    end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

AVFrame *FilterVideo::filter_operate(AVFrame *frame) {
    AVFrame *filt_frame = av_frame_alloc();
    /* push the decoded frame into the filtergraph */
    if (av_buffersrc_add_frame_flags(this->buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
        return NULL;
    }
    av_buffersink_get_frame(this->buffersink_ctx, filt_frame);

    return filt_frame;
}

int FilterVideo::init_filter_drawtext(char *args, char *fontfile, char *fontcolor, int fontsize, int box,
                                      char *boxcolor, char *text, int x, int y) {
    char filter_descr[512];
    snprintf(filter_descr, sizeof(filter_descr),
             "drawtext=fontfile=%s:fontcolor=%s:fontsize=%d:box=%d:boxcolor=%s:text=%s:x=%d:y=%d",
             fontfile, fontcolor, fontsize, box, boxcolor, text, x, y);
    if (this->init_filter(filter_descr, args) < 0)
        return -1;
    return 0;
}

int FilterVideo::update_filter_drawtext_text(char *text) {
    const char *target = "drawtext";
    const char *cmd = "reinit";
    char arg[512];
    snprintf(arg, sizeof(arg), "text=%s", text);
    char res[512];
    int ret = 0;
    ret = avfilter_graph_send_command(this->filter_graph, target, cmd, arg, res, 512, 1);
    return ret;
}

int FilterVideo::update_filter_drawtext_x_y(int x, int y) {
    const char *target = "drawtext";
    const char *cmd = "reinit";
    char arg[512];
    snprintf(arg, sizeof(arg), "x=%d:y=%d", x, y);
    char res[512];
    int ret = 0;
    ret = avfilter_graph_send_command(this->filter_graph, target, cmd, arg, res, 512, 1);
    return ret;
}

int FilterVideo::init_filter_crop(char *args, char *iw, char *ih, int x, int y) {
    char filter_descr[512];
    snprintf(filter_descr, sizeof(filter_descr), "crop=%s:%s:%d:%d", iw, ih, x, y);
    if (init_filter(filter_descr, args) < 0)
        return -1;
    return 0;
}

int FilterVideo::init_filter_scale(char *args, char *iw, char *ih) {
    char filter_descr[512];
    snprintf(filter_descr, sizeof(filter_descr), "scale=%s:%s", iw, ih);
    if (init_filter(filter_descr, args) < 0)
        return -1;
    return 0;
}

int FilterVideo::init_filter_scale(char *args, int w, int h) {
    char filter_descr[512];
    snprintf(filter_descr, sizeof(filter_descr), "scale=%d:%d", w, h);
    if (init_filter(filter_descr, args) < 0)
        return -1;
    return 0;
}

int FilterVideo::init_filter_add_picture(char *args, char *picturename, int x, int y) {
    char filter_descr[512];
    snprintf(filter_descr, sizeof(filter_descr), "movie=%s[wm];[in][wm]overlay=x=%d:y=%d[out]", picturename, x, y);
    if (init_filter(filter_descr, args) < 0)
        return -1;
    return 0;
}

int FilterVideo::init_filter_xfade(char *args, char *moviename, char *transition_type, int duration, int offset) {
    char filter_descr[512];
    snprintf(filter_descr, sizeof(filter_descr), "movie=%s[wm];[in][wm]xfade=transition=%s:duration=%d:offset=%d[out]",
             moviename, transition_type, duration, offset);
    if (init_filter(filter_descr, args) < 0)
        return -1;
    return 0;
}