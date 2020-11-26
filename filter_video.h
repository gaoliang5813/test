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

    int init_filtering_drawtext(char *args, char *fontfile = "FreeSans", char *fontcolor = "blue@0.5", int fontsize = 100, int box = 1,
                                char *boxcolor = "red@0.5", char *text = "hello", int x = 10, int y = 10);

    int update_filters_drawtext(char *arg);

    int init_filtering_crop(char *args, char *filter_type, char *iw, char *ih);

    AVFrame *filtering_drawtext(AVFrame *frame);

private:
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;

    int init_filters(const char *filters_descr, char *args);
};

#endif
