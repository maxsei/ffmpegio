#include "ffmpegio.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <stdbool.h>
#include <stdio.h>

#include "libavutil/imgutils.h"
#include "panic.h"

void ffmpegio_init(FFMPEGIOContext *ctx) {
  ctx->video_stream = -1;
  ctx->input_ctx = NULL;
  ctx->decoder = NULL;
  ctx->decoder_ctx = NULL;
  ctx->packet = NULL;
  ctx->packet_valid = false;
  ctx->want_new_packet = true;
}

FFMPEGIOError ffmpegio_open(FFMPEGIOContext *ctx, char *filepath) {
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
  av_register_all();
#endif

  // Find best input video stream.
  if (avformat_open_input(&(ctx->input_ctx), filepath, NULL, NULL) != 0)
    return FFMPEGIO_ERROR_AVFORMAT_OPEN_INPUT;
  if (avformat_find_stream_info(ctx->input_ctx, NULL) < 0)
    return FFMPEGIO_ERROR_AVFORMAT_FIND_STREAM_INFO;
  ctx->video_stream = av_find_best_stream(ctx->input_ctx, AVMEDIA_TYPE_VIDEO,
                                          -1, -1, &(ctx->decoder), 0);
  if (ctx->video_stream < 0) return FFMPEGIO_ERROR_AV_FIND_BEST_STREAM;

  // Allocate decoder context based on input video stream.
  ctx->decoder_ctx = avcodec_alloc_context3(ctx->decoder);
  if (!(ctx->decoder_ctx)) return FFMPEGIO_ERROR_AVCODEC_ALLOC_CONTEXT3;
  if (avcodec_parameters_to_context(
          ctx->decoder_ctx,
          ctx->input_ctx->streams[ctx->video_stream]->codecpar) < 0)
    return FFMPEGIO_ERROR_AVCODEC_PARAMETERS_TO_CONTEXT;

  // Open video stream.
  if (avcodec_open2(ctx->decoder_ctx, ctx->decoder, NULL) < 0)
    return FFMPEGIO_ERROR_AVCODEC_OPEN2;
  return FFMPEGIO_ERROR_NONE;
}

FFMPEGIOError ffmpegio_read(FFMPEGIOContext *ctx, AVFrame *frame) {
  // Allocate packet if not allocated.
  if (ctx->packet == NULL) {
    ctx->packet = (AVPacket *)(malloc(sizeof(AVPacket)));
  }

  if (!(ctx->packet_valid || ctx->want_new_packet)) {
    return FFMPEGIO_ERROR_EOF;
  }
  // prepare frame and packet for re-use
  if (ctx->packet_valid) {
    av_packet_unref(ctx->packet);
    ctx->packet_valid = false;
  }

  // read compressed data from stream and send it to the decoder
  if (ctx->want_new_packet) {
    // end of stream
    if (av_read_frame(ctx->input_ctx, ctx->packet) < 0)
      return FFMPEGIO_ERROR_EOF;
    ctx->packet_valid = true;
    if (ctx->packet->stream_index != ctx->video_stream)
      return FFMPEGIO_ERROR_SKIP;  // not a video packet
    // video packet
    if (avcodec_send_packet(ctx->decoder_ctx, ctx->packet) < 0)
      return FFMPEGIO_ERROR_AVCODEC_SEND_PACKET;

    ctx->want_new_packet = false;
  }

  // retrieve a frame from the decoder.
  int ret = avcodec_receive_frame(ctx->decoder_ctx, frame);
  if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF)) {
    ctx->want_new_packet = true;
    return FFMPEGIO_ERROR_SKIP;  // no more frames ready from the decoder ->
                                 // decode new ones
    // from the decoder -> decode new ones
  } else if (ret < 0) {
    return FFMPEGIO_ERROR_AVCODEC_RECEIVE_FRAME;
  }

  return FFMPEGIO_ERROR_NONE;
}

FFMPEGIOError ffmpegio_skip(FFMPEGIOContext *ctx) { return 0; }

void ffmpegio_frame_rgba_decode(AVFrame *frame, uint8_t *dst) {
  // Setup context for resizing/reformatting frame.
  struct SwsContext *sw_ctx =
      sws_getContext(frame->width, frame->height, frame->format, frame->width,
                     frame->height, AV_PIX_FMT_RGBA, 0, NULL, NULL, NULL);
  // Resize/reformat frame.
  // 32 bbp for rgba divided by 8 bits per byte times width of image.
  int dst_stride[1] = {(32 / 8) * frame->width};
  uint8_t *dst_slice[1] = {dst};
  sws_scale(sw_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0,
            frame->height, dst_slice, dst_stride);
}

FFMPEGIOError ffmpegio_close(FFMPEGIOContext *ctx) {
  if (ctx->packet != NULL) av_packet_unref(ctx->packet);
  if (ctx->input_ctx != NULL) avformat_close_input(&(ctx->input_ctx));
  if (ctx->decoder_ctx != NULL) avcodec_free_context(&(ctx->decoder_ctx));
  // av_frame_free(&frame);
  // if (ctx != NULL)
  // 	free(ctx);
  return 0;
}

const char *ffmpegio_error(FFMPEGIOError err) {
  switch (err) {
    case FFMPEGIO_ERROR_AVFORMAT_OPEN_INPUT:
      return "FFMPEGIO_ERROR_AVFORMAT_OPEN_INPUT";
    case FFMPEGIO_ERROR_AVFORMAT_FIND_STREAM_INFO:
      return "FFMPEGIO_ERROR_AVFORMAT_FIND_STREAM_INFO";
    case FFMPEGIO_ERROR_AV_FIND_BEST_STREAM:
      return "FFMPEGIO_ERROR_AV_FIND_BEST_STREAM";
    case FFMPEGIO_ERROR_AVCODEC_ALLOC_CONTEXT3:
      return "FFMPEGIO_ERROR_AVCODEC_ALLOC_CONTEXT3";
    case FFMPEGIO_ERROR_AVCODEC_PARAMETERS_TO_CONTEXT:
      return "FFMPEGIO_ERROR_AVCODEC_PARAMETERS_TO_CONTEXT";
    case FFMPEGIO_ERROR_AVCODEC_OPEN2:
      return "FFMPEGIO_ERROR_AVCODEC_OPEN2";
    case FFMPEGIO_ERROR_AVCODEC_SEND_PACKET:
      return "FFMPEGIO_ERROR_AVCODEC_SEND_PACKET";
    case FFMPEGIO_ERROR_AVCODEC_RECEIVE_FRAME:
      return "FFMPEGIO_ERROR_AVCODEC_RECEIVE_FRAME";
    case FFMPEGIO_ERROR_NONE:
      return "FFMPEGIO_ERROR_NONE";
    case FFMPEGIO_ERROR_EOF:
      return "FFMPEGIO_ERROR_EOF";
    case FFMPEGIO_ERROR_SKIP:
      return "FFMPEGIO_ERROR_SKIP";
      // default: panicf("no such error code %d", err);
  }
  return "FFMPEGIO_ERROR_UNKNOWN";
}
