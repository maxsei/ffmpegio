#include <stdbool.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#ifndef _FFMPEGIO_H
#define _FFMPEGIO_H

typedef enum FFMPEGIOError {
	FFMPEGIO_ERROR_AVFORMAT_OPEN_INPUT = -8,
	FFMPEGIO_ERROR_AVFORMAT_FIND_STREAM_INFO,
	FFMPEGIO_ERROR_AV_FIND_BEST_STREAM,
	FFMPEGIO_ERROR_AVCODEC_ALLOC_CONTEXT3,
	FFMPEGIO_ERROR_AVCODEC_PARAMETERS_TO_CONTEXT,
	FFMPEGIO_ERROR_AVCODEC_OPEN2,
	FFMPEGIO_ERROR_AVCODEC_SEND_PACKET,
	FFMPEGIO_ERROR_AVCODEC_RECEIVE_FRAME,

	FFMPEGIO_ERROR_NONE,
	FFMPEGIO_ERROR_EOF,
	FFMPEGIO_ERROR_SKIP,
} FFMPEGIOError;

const char* ffmpegio_error(FFMPEGIOError err);


typedef struct FFMPEGIOContext{
	int             video_stream;
	AVFormatContext *input_ctx;
	AVCodec	        *decoder;
	AVCodecContext  *decoder_ctx;
	AVPacket        *packet;
	bool            packet_valid; //TODO: make private
	bool            want_new_packet; //TODO: make private
} FFMPEGIOContext;

void ffmpegio_init(FFMPEGIOContext *ctx);
FFMPEGIOError ffmpegio_open(FFMPEGIOContext *ctx, char *filepath);
FFMPEGIOError ffmpegio_read(FFMPEGIOContext *ctx, AVFrame *frame);
FFMPEGIOError ffmpegio_skip(FFMPEGIOContext *ctx);
FFMPEGIOError ffmpegio_close(FFMPEGIOContext *ctx);

#endif
