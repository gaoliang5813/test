extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include <libavutil/opt.h>
}
#include "filter_video.h"
#include <iostream>

using namespace std;

static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
static int video_stream_index = -1;

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

FILE *file_fd;

static void write_frame(const AVFrame *frame) {
//    if (frame->format == AV_PIX_FMT_YUV420P) {
//        printf("format is yuv420p\n");
//    } else {
//        printf("format is not yuv420p\n");
//    }
//
//    printf("frame widht=%d,frame height=%d\n", frame->width, frame->height);

    fwrite(frame->data[0], 1, frame->width * frame->height, file_fd);
    fwrite(frame->data[1], 1, frame->width / 2 * frame->height / 2, file_fd);
    fwrite(frame->data[2], 1, frame->width / 2 * frame->height / 2, file_fd);
}

int main(int argc, char **argv) {

    char input_file[] = "coverr2.mp4";
    cout << "input file: " << input_file << endl;

    FilterVideo filter_drawtext;
    FilterVideo filter_scale;


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
    if ((ret = open_input_file(input_file)) < 0) {
        cout << "open input file failed" << endl;
    }

    char args[512];
    AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;
    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
             time_base.num, time_base.den,
             dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);


    if ((ret = filter_drawtext.init_filtering_drawtext(args)) >= 0) {
        cout << "init filtering drawtext successful" << endl;
    };

    int frame_count = 0;
    while (1) {
        frame_count++;
        if (frame_count > 100) {
            if ((ret = filter_drawtext.update_filters_drawtext()) >= 0) {
                cout << "update filtering drawtext successful" << endl;
            } else {
                cout << "update filtering drawtext failed" << endl;
            }
            frame_count = 0;
        }
        if ((ret = av_read_frame(fmt_ctx, &packet)) < 0)
            break;

        if (packet.stream_index == video_stream_index) {
            got_frame = 0;
            ret = avcodec_decode_video2(dec_ctx, frame, &got_frame, &packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error decoding video\n");
                break;
            }

            if (got_frame) {
                frame->pts = av_frame_get_best_effort_timestamp(frame);

                //filter_1
                filt_frame = filter_drawtext.filtering_drawtext(frame);

                //filter_2


                write_frame(filt_frame);
                av_frame_unref(filt_frame);
                av_frame_unref(frame);
            }
        }
        av_free_packet(&packet);
    }


    avcodec_close(dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);

    fclose(file_fd);

    cout << "Press any key to exit..." << endl;
    getwchar();
    return 0;
}
