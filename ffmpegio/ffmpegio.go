package ffmpegio

/*
#include <stdlib.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "ffmpegio.h"
*/
import "C"
import (
	"errors"
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

	// Initialize packet and set valid.
	ctx.ctx.packet = (*C.AVPacket)(C.malloc(C.size_t(unsafe.Sizeof(C.AVPacket{}))))
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
	if err == GoFFMPEGIO_ERROR_NONE {
		return nil
	}
	return err
}
func (ctx *Context) Skip() error {
	return FFMPEGIOError(0)
}
func (ctx *Context) Close() error {
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
	f.valid = false
	C.av_frame_free(&f.frame)
	return FFMPEGIOError(0)
}