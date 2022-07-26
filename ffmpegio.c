#include <stdio.h>
#include <stdbool.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "panic.h"
#include "ffmpegio.h"

void ffmpegio_init(FFMPEGIOContext *ctx)
{
	ctx->video_stream 		= -1;
	ctx->input_ctx 			= NULL;
	ctx->decoder 			= NULL;
	ctx->decoder_ctx 		= NULL;
	ctx->packet 			= NULL;
	ctx->packet_valid 		= false;
	ctx->want_new_packet 	= true;
}

FFMPEGIOError ffmpegio_open(FFMPEGIOContext *ctx, char *filepath)
{
	#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
		av_register_all();
	#endif

	// Find best input video stream.
	if (avformat_open_input(&(ctx->input_ctx), filepath, NULL, NULL) != 0)
		return FFMPEGIO_ERROR_AVFORMAT_OPEN_INPUT;
	if (avformat_find_stream_info(ctx->input_ctx, NULL) < 0)
		return FFMPEGIO_ERROR_AVFORMAT_FIND_STREAM_INFO;
	ctx->video_stream = av_find_best_stream(ctx->input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &(ctx->decoder), 0);
	if (ctx->video_stream < 0)
		return FFMPEGIO_ERROR_AV_FIND_BEST_STREAM;
	// Allocate decoder context based on input video stream.
	ctx->decoder_ctx = avcodec_alloc_context3(ctx->decoder);
	if (!(ctx->decoder_ctx))
		return FFMPEGIO_ERROR_AVCODEC_ALLOC_CONTEXT3;
	if (avcodec_parameters_to_context(ctx->decoder_ctx, ctx->input_ctx->streams[ctx->video_stream]->codecpar) < 0)
		return FFMPEGIO_ERROR_AVCODEC_PARAMETERS_TO_CONTEXT;

	// Open video stream.
	if (avcodec_open2(ctx->decoder_ctx, ctx->decoder, NULL) < 0)
		return FFMPEGIO_ERROR_AVCODEC_OPEN2;
	return FFMPEGIO_ERROR_NONE;
}

FFMPEGIOError ffmpegio_read(FFMPEGIOContext *ctx, AVFrame *frame)
{
	if(!(ctx->packet_valid || ctx->want_new_packet)){
		return FFMPEGIO_ERROR_EOF;
	}
	// prepare frame and packet for re-use
	if (ctx->packet_valid) { av_packet_unref(ctx->packet); ctx->packet_valid = false; }

	// read compressed data from stream and send it to the decoder
	if (ctx->want_new_packet) {
		 // end of stream
		if (av_read_frame(ctx->input_ctx, ctx->packet) < 0)
			return FFMPEGIO_ERROR_EOF;
		ctx->packet_valid = true;
		if (ctx->packet->stream_index != ctx->video_stream)
			return FFMPEGIO_ERROR_SKIP; // continue;  // not a video packet
			// return ffmpegio_read(ctx, frame); // return 0 // continue;  // not a video packet
		if (avcodec_send_packet(ctx->decoder_ctx, ctx->packet) < 0)
			return FFMPEGIO_ERROR_AVCODEC_SEND_PACKET;

		ctx->want_new_packet = false;
	}

	// retrieve a frame from the decoder.
	int ret = avcodec_receive_frame(ctx->decoder_ctx, frame);
	if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF)) {
		ctx->want_new_packet = true;
		return FFMPEGIO_ERROR_SKIP; // continue;  // no more frames ready from the decoder -> decode new ones
		// return ffmpegio_read(ctx, frame); // continue;  // no more frames ready from the decoder -> decode new ones
	}
	else if (ret < 0) {
		return FFMPEGIO_ERROR_AVCODEC_RECEIVE_FRAME;
	}

	// Show count on the same line.
	// printf("\rframe #%d (%c) ", ++frameno, av_get_picture_type_char(frame->pict_type));
	// fflush(stdout);
	return FFMPEGIO_ERROR_NONE;
}

FFMPEGIOError ffmpegio_skip(FFMPEGIOContext *ctx)
{
	return 0;
}
FFMPEGIOError ffmpegio_close(FFMPEGIOContext *ctx)
{
	if (ctx->packet != NULL)
		av_packet_unref(ctx->packet); 
	if (ctx->input_ctx != NULL)
		avformat_close_input(&(ctx->input_ctx));
	if (ctx->decoder_ctx != NULL)
		avcodec_free_context(&(ctx->decoder_ctx));
	// av_frame_free(&frame);
	// if (ctx != NULL)
	// 	free(ctx);
	return 0;
}

const char* fmmpegio_error(FFMPEGIOError err)
{
	switch(err){
		case FFMPEGIO_ERROR_AVFORMAT_OPEN_INPUT: return "FFMPEGIO_ERROR_AVFORMAT_OPEN_INPUT";
		case FFMPEGIO_ERROR_AVFORMAT_FIND_STREAM_INFO: return "FFMPEGIO_ERROR_AVFORMAT_FIND_STREAM_INFO";
		case FFMPEGIO_ERROR_AV_FIND_BEST_STREAM: return "FFMPEGIO_ERROR_AV_FIND_BEST_STREAM";
		case FFMPEGIO_ERROR_AVCODEC_ALLOC_CONTEXT3: return "FFMPEGIO_ERROR_AVCODEC_ALLOC_CONTEXT3";
		case FFMPEGIO_ERROR_AVCODEC_PARAMETERS_TO_CONTEXT: return "FFMPEGIO_ERROR_AVCODEC_PARAMETERS_TO_CONTEXT";
		case FFMPEGIO_ERROR_AVCODEC_OPEN2: return "FFMPEGIO_ERROR_AVCODEC_OPEN2";
		case FFMPEGIO_ERROR_AVCODEC_SEND_PACKET: return "FFMPEGIO_ERROR_AVCODEC_SEND_PACKET";
		case FFMPEGIO_ERROR_AVCODEC_RECEIVE_FRAME: return "FFMPEGIO_ERROR_AVCODEC_RECEIVE_FRAME";
		case FFMPEGIO_ERROR_NONE: return "FFMPEGIO_ERROR_NONE";
		case FFMPEGIO_ERROR_EOF: return "FFMPEGIO_ERROR_EOF";
		case FFMPEGIO_ERROR_SKIP: return "FFMPEGIO_ERROR_SKIP";
		default: panicf("no such error code %d", err);
	}
	return "";
}
