#include <stdio.h>
#include <stdbool.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

int panicf(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	const int ret = fprintf(stderr, fmt, argp);
	if (ret != 0)
		exit(ret);
	va_end(argp);
	exit(1);
}

int panic(const char *msg)
{
	return panicf(msg);
}

int main(int argc, char* argv[]){
	if (argc < 2)
		fprintf(stderr, "usage: %s <input.mp4>", argv[0]);

	// Setup input context, decoder, and decoder context.
	int             video_stream = -1;
	AVFormatContext *input_ctx   = NULL;
	AVCodec	        *decoder     = NULL;
	AVCodecContext  *decoder_ctx = NULL;

	#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
		av_register_all();
	#endif
	// Find best input video stream.
	if (avformat_open_input(&input_ctx, argv[1], NULL, NULL) != 0)
		panic("avformat_open_input");
	if (avformat_find_stream_info(input_ctx, NULL) < 0)
		panic("avformat_find_stream_info");
	video_stream = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
	if (video_stream < 0)
		panic("av_find_best_stream");
	// Allocate decoder context based on input video stream.
	decoder_ctx = avcodec_alloc_context3(decoder);
	if (!decoder_ctx)
		panic("avcodec_alloc_context3");
	if (avcodec_parameters_to_context(decoder_ctx, input_ctx->streams[video_stream]->codecpar) < 0)
		panic("avcodec_parameters_to_context");

	// Open video stream.
	if (avcodec_open2(decoder_ctx, decoder, NULL) < 0)
		panic("avcodec_open2");
	printf("Opened input video stream: %dx%d\n", decoder_ctx->width, decoder_ctx->height);

	// Extract frames.
	AVPacket packet;
	AVFrame  *frame = av_frame_alloc();
	if (!frame)
		panic("av_frame_alloc");
	int  frameno         = 0;
	bool packet_valid    = false;
	bool want_new_packet = true;
	for(; packet_valid || want_new_packet ;){
		// prepare frame and packet for re-use
		if (packet_valid) { av_packet_unref(&packet); packet_valid = false; }

		// read compressed data from stream and send it to the decoder
		if (want_new_packet) {
			 // end of stream
			if (av_read_frame(input_ctx, &packet) < 0)
				break; 
			packet_valid = true;
			if (packet.stream_index != video_stream)
				continue;  // not a video packet
			if (avcodec_send_packet(decoder_ctx, &packet) < 0)
				panic("avcodec_send_packet");
			want_new_packet = false;
		}

		// retrieve a frame from the decoder.
		int ret = avcodec_receive_frame(decoder_ctx, frame);
		if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF)) {
			want_new_packet = true;
			continue;  // no more frames ready from the decoder -> decode new ones
		}
		else if (ret < 0) {
			panic("avcodec_receive_frame");
		}

		// Show count on the same line.
		printf("\rframe #%d (%c) ", ++frameno, av_get_picture_type_char(frame->pict_type));
		fflush(stdout);
	}
	printf("\n%s has %d frame(s)\n", argv[1], frameno);

	// Cleanup.
	av_packet_unref(&packet); 
	avformat_close_input(&input_ctx);
	avcodec_free_context(&decoder_ctx);
	av_frame_free(&frame);
	return 0;
}
