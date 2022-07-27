#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdbool.h>
#include <stdio.h>

#include "ffmpegio/panic.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <input.mp4>", argv[0]);
    return 2;
  }

  int video_stream = -1;
  AVFormatContext *input_ctx = NULL;
  AVCodec *decoder = NULL;
  AVCodecContext *decoder_ctx = NULL;
  AVPacket packet;

  // Find best input video stream.
  if (avformat_open_input(&input_ctx, argv[1], NULL, NULL) != 0)
    panic("avformat_open_input");
  if (avformat_find_stream_info(input_ctx, NULL) < 0)
    panic("avformat_find_stream_info");
  video_stream =
      av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &(decoder), 0);
  if (video_stream < 0) panic("av_find_best_stream");
  // Allocate decoder context based on input video stream.
  decoder_ctx = avcodec_alloc_context3(decoder);
  if (!(decoder_ctx)) panic("avcodec_alloc_context3");
  if (avcodec_parameters_to_context(
          decoder_ctx, input_ctx->streams[video_stream]->codecpar) < 0)
    panic("avcodec_parameters_to_context");

  // Open video stream.
  if (avcodec_open2(decoder_ctx, decoder, NULL) < 0) panic("avcodec_open2");

  // allocate AVFrame for display
  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    panic("av_frame_alloc");
  }

  int frameno = 0;
  int ret;
  for (;;) {
    // Get packet with correct video stream from input context.
    for (;;) {
      if (av_read_frame(input_ctx, &packet) < 0) return 0;
      if (packet.stream_index == video_stream) break;
    }
    // Decode packet.
    if (avcodec_send_packet(decoder_ctx, &packet) < 0)
      panic("avcodec_send_packet");
    // Read frames from decoder.
    for (;;) {
      ret = avcodec_receive_frame(decoder_ctx, frame);
      if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF)) break;
      if (ret < 0) panic("avcodec_receive_frame");

      printf("\rframe Ct: %d frame #%d (%c) ", ++frameno,
             frame->coded_picture_number,
             av_get_picture_type_char(frame->pict_type));
      fflush(stdout);
    }
    // Dereference packet.
    av_packet_unref(&packet);
  }
  return 0;
}
