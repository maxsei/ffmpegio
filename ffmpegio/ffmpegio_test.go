package ffmpegio

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestInvalidPath(t *testing.T) {
	// Open context to file.
	_, err := OpenContext("./some/non/existent/filepath/65563e51-28c1-4870-bf7a-4bef8112662b.mp4")
	assert.Equal(t, GoFFMPEGIO_ERROR_AVFORMAT_OPEN_INPUT, err)
	// defer ctx.Close()
}

func TestFramecounter(t *testing.T) {
	// ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of csv=p=0 ./sample.mp4
	const frameCountExpected int = 4

	// Open context to file.
	ctx, err := OpenContext("../sample.mp4")
	if err != nil {
		t.Error(err)
	}
	defer ctx.Close()

	// Open Frames to read.
	frame, err := NewFrame()
	if err != nil {
		t.Error(err)
	}
	defer frame.Close()

	// Count frames and get max frame number.
	var maxFrameNum int = -1
	var frameCount int
loop:
	for {
		switch err := ctx.Read(frame); err {
		case GoFFMPEGIO_ERROR_NONE, nil:
			frameCount++
			frameNum := int(frame.frame.coded_picture_number)
			if frameNum > maxFrameNum {
				maxFrameNum = frameNum
			}
		case GoFFMPEGIO_ERROR_SKIP:
			continue
		case GoFFMPEGIO_ERROR_EOF:
			break loop
		default:
			t.Error(err)
		}
	}

	// Check number of actual frames in picture.
	assert.Equal(t, frameCountExpected, maxFrameNum+1, "maxFrameNum + 1")
	assert.Equal(t, frameCountExpected, frameCount, "frameCount")
}
