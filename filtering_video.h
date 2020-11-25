#ifndef FILTERING_VIDEO_H
#define FILTERING_VIDEO_H

#define _XOPEN_SOURCE 600 /* for usleep */

#include <unistd.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
//#include <libavfilter/avfiltergraph.h>
//#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>

int init_filtering_drawtext();
int update_filters_drawtext();
AVFrame *filtering_drawtext(AVFrame *frame);

#endif
