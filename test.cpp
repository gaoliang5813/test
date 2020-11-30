extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/mem.h>
#include <libavutil/imgutils.h>
}


#include "funset.hpp"
#include <stdio.h>
#include <iostream>
#include <memory>
#include <fstream>
 


 
#include <opencv2/opencv.hpp>
 
int test_ffmpeg_decode_show_new()
{
	avdevice_register_all();
 
	AVDictionary* options = NULL;
#ifdef _MSC_VER
	const char* input_format_name = "vfwcap";
	const char* url = "";
#else
	const char* input_format_name = "video4linux2";
	const char* url = "/dev/video0";
	av_dict_set(&options, "video_size", "640x480", 0);
	av_dict_set(&options, "input_format", "mjpeg", 0);
#endif
 
	AVInputFormat* input_fmt = av_find_input_format(input_format_name);
	AVFormatContext* format_ctx = avformat_alloc_context();
 
	int ret = avformat_open_input(&format_ctx, url, input_fmt, &options);
	if (ret != 0) {
		fprintf(stderr, "fail to open url: %s, return value: %d\n", url, ret);
		return -1;
	}
 
	ret = avformat_find_stream_info(format_ctx, NULL);
	if (ret < 0) {
		fprintf(stderr, "fail to get stream information: %d\n", ret);
		return -1;
	}
 
	int video_stream_index = -1;
	for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
		const AVStream* stream = format_ctx->streams[i];
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = i;
			fprintf(stdout, "type of the encoded data: %d, dimensions of the video frame in pixels: width: %d, height: %d, pixel format: %d\n",
				stream->codecpar->codec_id, stream->codecpar->width, stream->codecpar->height, stream->codecpar->format);
		}
	}
 
	if (video_stream_index == -1) {
		fprintf(stderr, "no video stream\n");
		return -1;
	}
 
	AVCodecParameters* codecpar = format_ctx->streams[video_stream_index]->codecpar;
	const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
	if (!codec) {
		fprintf(stderr, "fail to avcodec_find_decoder\n");
		return -1;
	}
 
	AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
	if (!codec_ctx) {
		fprintf(stderr, "fail to avcodec_alloc_context3\n");
		return -1;
	}
 
	codec_ctx->pix_fmt = AVPixelFormat(codecpar->format);
	codec_ctx->height = codecpar->height;
	codec_ctx->width = codecpar->width;
	codec_ctx->thread_count = 4;
	ret = avcodec_open2(codec_ctx, codec, NULL);
	if (ret != 0) {
		fprintf(stderr, "fail to avcodec_open2: %d\n", ret);
		return -1;
	}
 
	AVFrame* frame = av_frame_alloc();
	AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	SwsContext* sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, AV_PIX_FMT_BGR24, 0, NULL, NULL, NULL);
	if (!frame || !packet || !sws_ctx) {
		fprintf(stderr, "fail to alloc\n");
		return -1;
	}
 
	uint8_t* bgr_data[4];
	int bgr_linesize[4];
	av_image_alloc(bgr_data, bgr_linesize, codec_ctx->width, codec_ctx->height, AV_PIX_FMT_BGR24, 1);
	cv::Mat mat(codec_ctx->height, codec_ctx->width, CV_8UC3);
	const char* winname = "usb video2";
	cv::namedWindow(winname);
 
	while (1) {
		ret = av_read_frame(format_ctx, packet);
		if (ret >= 0 && packet->stream_index == video_stream_index) {
			ret = avcodec_send_packet(codec_ctx, packet);
			if (ret < 0) {
				fprintf(stderr, "fail to avcodec_send_packet: %d\n", ret);
				av_packet_unref(packet);
				continue;
			}
 
			ret = avcodec_receive_frame(codec_ctx, frame);
			if (ret < 0) {
				fprintf(stderr, "fail to avcodec_receive_frame\n");
				av_packet_unref(packet);
				continue;
			}
 
			sws_scale(sws_ctx, frame->data, frame->linesize, 0, codec_ctx->height, bgr_data, bgr_linesize);
			mat.data = bgr_data[0];
			cv::imshow(winname, mat);
		}
 
		av_packet_unref(packet);
 
		int key = cv::waitKey(25);
		if (key == 27) break;
	}
 
	cv::destroyWindow(winname);
	av_frame_free(&frame);
	sws_freeContext(sws_ctx);
	av_dict_free(&options);
	avformat_close_input(&format_ctx);
	av_freep(packet);
	av_freep(&bgr_data[0]);
 
	fprintf(stdout, "test finish\n");
	return 0;
}

