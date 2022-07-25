#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#define nullptr ((void *)0)
#define INBUF_SIZE 4096 * 8

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

void list_codecs()
{
	void* iterate_data = NULL;
	const AVCodec* current_codec = av_codec_iterate(&iterate_data);
	while (current_codec != NULL)
	{
		printf("name: %s long_name: %s\n", current_codec->name, current_codec->long_name);
		current_codec = av_codec_iterate(&iterate_data);
	}
}

static void decode(
	AVCodecContext *dec_ctx,
	AVFrame *frame,
	AVPacket *pkt
)
{
	const char *filename = "some-random-filenameeee";
	char buf[1024];
	int ret;
 
	ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0)
		panic("error sending a packet for decoding\n");
 
	while (ret >= 0) {
		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0)
			panic("error during decoding\n");
 
		printf("saving frame %3d\n", dec_ctx->frame_number);
		fflush(stdout);
 
		/* the picture is allocated by the decoder. no need to
		   free it */
		snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
	}
}

int main(){
	const char *filename = "vid.mp4";

	// Infer format from filename.
	// TODO: maybe try multiple formats to see what works?
	// const AVOutputFormat *format = av_guess_format("mp4", NULL, NULL);
	const AVOutputFormat *format = av_guess_format(NULL, filename, NULL);
	if (!format)
		panicf("couldn't infer vid format from filename %s", filename);

	// printf("format->video_codec: %u enum: %u\n", format->video_codec, AV_CODEC_ID_H264);

	// Find the codec.
	// const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	const AVCodec *codec = avcodec_find_decoder(format->video_codec);
	if (!codec)
		panic("codec not found");


	// Init parser.
	AVCodecParserContext *parser = av_parser_init(codec->id);
	if (!parser)
		panic("parser not found");

	// Inic codec context.
	AVCodecContext *c = avcodec_alloc_context3(codec);
	if (!c)
		panic("could not allocate video codec context");

	// Open codec.
	if (avcodec_open2(c, codec, NULL) < 0)
		panic("could not open codec");

	// Open video file.
	FILE *f = fopen(filename, "rb");
	if (!f)
		panicf("could not open %s", filename);

	 // Allocate frame.
	 AVFrame *frame = av_frame_alloc();
	 if (!frame)
	 	panic("could not allocate video frame");

	// Decode frames.
	uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	size_t data_size;
	int eof;
	uint8_t *data;
	int ret;

	// Allocate packet.
	AVPacket *pkt = av_packet_alloc();
	if (!pkt)
		panic("couldn't allocate packet");

	// set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams)
	memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

	do {
		/* read raw data from the input file */
		data_size = fread(inbuf, 1, INBUF_SIZE, f);
		if (ferror(f))
			break;
		eof = !data_size;
 
		/* use the parser to split the data into frames */
		data = inbuf;
		while (data_size > 0 || eof) {
			ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
								   data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			if (ret < 0)
				panic("error while parsing\n");

			data	  += ret;
			data_size -= ret;
 
			if (pkt->size)
				decode(c, frame, pkt);
			else if (eof)
				break;
		}
	} while (!eof);



	// TODO: free resources.
	fclose(f);
	return 0;
}
