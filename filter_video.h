#ifndef FILTER_VIDEO_H
#define FILTER_VIDEO_H

#define _XOPEN_SOURCE 600 /* for usleep */

#include <unistd.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
}

class FilterVideo {
public:
//    FilterVideo();
//
//    ~FilterVideo();

    AVFrame *filter_operate(AVFrame *frame);

    int init_filter_drawtext(char *args, char *fontfile = "FreeSans", char *fontcolor = "blue@0.5", int fontsize = 100, int box = 1,
                                char *boxcolor = "red@0.5", char *text = "hello", int x = 10, int y = 10);

    int update_filter_drawtext_text(char *text);
    int update_filter_drawtext_x_y(int x,int y);

    int init_filter_crop(char *args, char *iw, char *ih,int x,int y);

    int init_filter_scale(char *args, char *iw, char *ih);
    int init_filter_scale(char *args, int w, int h);

    int init_filter_add_picture(char *args, char *picturename, int x, int y);

    int init_filter_xfade(char *args, char *moviename, char *transition_type, int duration, int offset);

private:
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;

    int init_filter(const char *filters_descr, char *args);
};

#endif
