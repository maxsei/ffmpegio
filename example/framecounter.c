#include <stdio.h>
#include <stdbool.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <ffmpegio/ffmpegio.h>
#include <ffmpegio/panic.h>

int main(int argc, char* argv[]){
	if (argc < 2) {
		fprintf(stderr, "usage: %s <input.mp4>", argv[0]);
		return 2;
	}

	FFMPEGIOContext ctx;
	FFMPEGIOError err;

	ffmpegio_init(&ctx);
	err = ffmpegio_open(&ctx, argv[1]);
	if (err < 0)
		panic(ffmpegio_error(err));


	AVFrame *frame = av_frame_alloc();
	if (!frame)
		panic("av_frame_alloc");

	AVPacket packet;
	ctx.packet = &packet;

	for(;;)
	{
		err = ffmpegio_read(&ctx, frame);
		if (err < 0 || err == FFMPEGIO_ERROR_EOF)
				break;
		if (err == FFMPEGIO_ERROR_SKIP)
				continue;
		printf("\rframe #%d (%c) ", frame->coded_picture_number, av_get_picture_type_char(frame->pict_type));
		fflush(stdout);
	}

	err = ffmpegio_close(&ctx);
	if (err < 0)
		panic(ffmpegio_error(err));
}
