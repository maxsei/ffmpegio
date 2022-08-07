package ffmpegio

import (
	"testing"

	"bytes"
	"crypto/md5"
	"github.com/stretchr/testify/assert"
	"image/png"
	"fmt"
)

const samplePath = "../sample.mp4"

func TestContextClose(t *testing.T) {
	// Open context to file.
	ctx, err := OpenContext(samplePath)
	if err != nil {
		t.Fatal(t)
	}
	// Try closing context.
	defer func() {
		if r := recover(); r != nil {
			t.Fatalf("panic in ctx.Close(): %v", r)
		}
	}()
	ctx.Close()
}

func TestInvalidPath(t *testing.T) {
	// Open context to file.
	ctx, err := OpenContext("./some/non/existent/filepath/65563e51-28c1-4870-bf7a-4bef8112662b.mp4")
	assert.Equal(t, GoFFMPEGIO_ERROR_AVFORMAT_OPEN_INPUT, err)
	defer ctx.Close()
}

func TestFramecounter(t *testing.T) {
	// ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of csv=p=0 ./sample.mp4
	const frameCountExpected int = 4

	// Open context to file.
	ctx, err := OpenContext(samplePath)
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

func TestFrameImage(t *testing.T) {
	// Open context to file.
	ctx, err := OpenContext(samplePath)
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

	// Read in a single frame.
	if err := ctx.Read(frame); err != nil {
		t.Error(err)
	}

	// Extract the first frame as an image.
	img, err := ctx.Image(frame)
	if err != nil {
		t.Error(err)
	}

	// Encode data as
	buf := bytes.NewBuffer([]byte{})
	if err := png.Encode(buf, img); err != nil {
		t.Error(err)
	}

	actual := fmt.Sprintf("%x", md5.Sum(buf.Bytes()))
	// ffmpeg -i ./sample.mp4 -r 1 -f image2 ./sample-frame1.png && md5sum ./sample-frame1.png
	const expected string = "8a95fcbdc4c1418edaa2c2cf76d05c71"
	assert.Equal(t, expected, actual)
}
