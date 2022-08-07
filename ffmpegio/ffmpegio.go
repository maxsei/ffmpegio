package ffmpegio

/*
#cgo pkg-config: libavdevice libavformat libavfilter libavcodec libswresample libswscale libavutil
#include <stdlib.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "ffmpegio.h"
*/
import "C"
import (
	"errors"
	"image"
	"unsafe"
)

const (
	// BEGIN C FFMPEGIOError
	// Error
	GoFFMPEGIO_ERROR_AVFORMAT_OPEN_INPUT FFMPEGIOError = iota - 8
	GoFFMPEGIO_ERROR_AVFORMAT_FIND_STREAM_INFO
	GoFFMPEGIO_ERROR_AV_FIND_BEST_STREAM
	GoFFMPEGIO_ERROR_AVCODEC_ALLOC_CONTEXT3
	GoFFMPEGIO_ERROR_AVCODEC_PARAMETERS_TO_CONTEXT
	GoFFMPEGIO_ERROR_AVCODEC_OPEN2
	GoFFMPEGIO_ERROR_AVCODEC_SEND_PACKET
	GoFFMPEGIO_ERROR_AVCODEC_RECEIVE_FRAME
	// Ok
	GoFFMPEGIO_ERROR_NONE
	GoFFMPEGIO_ERROR_EOF
	GoFFMPEGIO_ERROR_SKIP
	// END C FFMPEGIOError

	// BEGIN Go FFMPEGIOError
	GoFFMPEGIO_ERROR_INVALID
	// END Go FFMPEGIOError
)

type FFMPEGIOError int

func (err FFMPEGIOError) Error() string {
	p := C.ffmpegio_error(C.FFMPEGIOError(err))
	s := C.GoString(p)
	return s
}

func OpenContext(filepath string) (*Context, error) {
	// Initialize context.
	ctx := &Context{
		ctx: (*C.FFMPEGIOContext)(C.malloc(C.size_t(unsafe.Sizeof(C.FFMPEGIOContext{})))),
	}
	C.ffmpegio_init(ctx.ctx)

	// C string filepath.
	bbFilepath := []byte(filepath)
	cFilepath := (*C.char)(unsafe.Pointer(&bbFilepath[0]))

	// Try opening ffmpegio context with filepath.
	ret := C.ffmpegio_open(ctx.ctx, cFilepath)
	err := FFMPEGIOError(ret)
	if err != GoFFMPEGIO_ERROR_NONE {
		return nil, err
	}

	// Set valid.
	ctx.valid = true
	return ctx, nil
}

type Context struct {
	ctx   *C.FFMPEGIOContext
	valid bool
}

func (ctx *Context) Valid(filepath string) bool { return ctx.valid }

func (ctx *Context) Read(frame *Frame) error {
	ret := C.ffmpegio_read(ctx.ctx, frame.frame)
	err := FFMPEGIOError(ret)
	switch err {
	case GoFFMPEGIO_ERROR_NONE:
		frame.valid = true
		return nil
	case GoFFMPEGIO_ERROR_SKIP:
		frame.valid = true
	default:
		frame.valid = false
	}
	return err
}

func (ctx *Context) Skip() error {
	return FFMPEGIOError(0)
}

// Image returns a copy of the image in the current frame.
func (ctx *Context) Image(frame *Frame) (image.Image, error) {
	if !frame.valid {
		return nil, GoFFMPEGIO_ERROR_INVALID
	}

	// Create a new image with the correct dimensions return if 0x0.
	rect := image.Rect(0, 0, int(frame.frame.width), int(frame.frame.height))
	ret := image.NewRGBA(rect)
	if len(ret.Pix) < 0 {
		return ret, nil
	}

	// Decode frame into image buffer as rgba data.
	C.ffmpegio_frame_rgba_decode(frame.frame, (*C.uint8_t)(unsafe.Pointer(&ret.Pix[0])))
	return ret, nil
}

func (ctx *Context) Close() error {
	if ctx == nil {
		return GoFFMPEGIO_ERROR_INVALID
	}
	if !ctx.valid {
		return GoFFMPEGIO_ERROR_INVALID
	}
	ctx.valid = false

	ret := C.ffmpegio_close((*C.FFMPEGIOContext)(unsafe.Pointer(ctx.ctx)))
	err := FFMPEGIOError(ret)
	return err
}

func NewFrame() (*Frame, error) {
	ret := Frame{frame: C.av_frame_alloc()}
	if ret.frame == nil {
		return nil, errors.New("failed to allocate frame")
	}
	return &ret, nil
}

type Frame struct {
	frame *C.AVFrame
	valid bool
}

func (f *Frame) Valid() bool { return f.valid }

func (f *Frame) Close() error {
	if f == nil {
		return GoFFMPEGIO_ERROR_INVALID
	}
	if !f.valid {
		return GoFFMPEGIO_ERROR_INVALID
	}
	f.valid = false
	C.av_frame_free(&f.frame)
	return FFMPEGIOError(0)
}
