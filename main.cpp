extern "C"
{
#include "filtering_video.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

#include <iostream>

using namespace std;

int main(int argc, char **argv) {
    cout << "ffmpeg info: " << avcodec_configuration() << endl;

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //char input_file[] = "鹿小草-年轮说.mp4";
    char input_file[] = "testvideo_1.mp4";
    cout << "input file: " << input_file << endl;

    if (avformat_open_input(&pFormatCtx, input_file, NULL, NULL) != 0) {
        printf("Couldn't open input stream.\n");
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Couldn't find stream information.\n");
        return -1;
    }
    int videoindex = -1;
    for (unsigned int i = 0; i < pFormatCtx->nb_streams; ++i) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    }
    if (videoindex == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }

    AVCodec *pCodec = avcodec_find_decoder(pFormatCtx->streams[videoindex]->codecpar->codec_id);
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar);
    if (pCodec == NULL) {
        printf("Codec not found.\n");
        return -1;
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codecpar.\n");
        return -1;
    }

    cout << "duration: " << pFormatCtx->duration << " us." << endl;
    cout << "format: " << pFormatCtx->iformat->name << ", long name: " << pFormatCtx->iformat->long_name << endl;
    cout << "image width: " << pFormatCtx->streams[videoindex]->codecpar->width << endl;
    cout << "image height: " << pFormatCtx->streams[videoindex]->codecpar->height << endl;
    cout << "nb_streams: " << pFormatCtx->nb_streams << endl;
    cout << "video_index: " << videoindex << endl;

    //Output Info-----------------------------
    printf("--------------- File Information ----------------\n");
    av_dump_format(pFormatCtx, 0, input_file, 0);
    printf("-------------------------------------------------\n");

    filtering_video(input_file);


    cout << "Press any key to exit..." << endl;
    getwchar();
    return 0;
}
