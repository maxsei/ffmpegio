package ffmpegio

import (
	"bytes"
	"crypto/md5"
	"encoding/gob"
	"fmt"
	"image"
	"testing"

	"github.com/stretchr/testify/assert"
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
	img, err := frame.ImageRGBA()
	if err != nil {
		t.Error(err)
	}
	// Rotate image 90 degrees.
	imgRot := image.NewRGBA(image.Rect(0, 0, img.Bounds().Max.Y, img.Bounds().Max.X))
	for i := 0; i < imgRot.Bounds().Max.X; i++ {
		for j := 0; j < imgRot.Bounds().Max.Y; j++ {
			imgRot.Set(i, j, img.At(j, img.Bounds().Max.Y-i-1))
		}
	}

	/*
				// Open sample image.
		    // ffmpeg -i ./sample.mp4 -r 1 -frames:v 1 ./sample-frame1.png
				f, err := os.Open("../sample-frame1.png")
				if err != nil {
					t.Error(err)
				}
				defer f.Close()
				imgSamp, err := png.Decode(f)
				if err != nil {
					t.Error(err)
				}
				buf := bytes.NewBuffer([]byte{})
				if err := gob.NewEncoder(buf).Encode(imgSamp); err != nil {
					t.Error(err)
				}
				t.Logf("%x\n", md5.Sum(buf.Bytes())) // 3e699d77f307633af199ab1748c25cf6
	*/
	const expected string = "3e699d77f307633af199ab1748c25cf6"

	buf := bytes.NewBuffer([]byte{})
	if err := gob.NewEncoder(buf).Encode(imgRot); err != nil {
		t.Error(err)
	}
	actual := fmt.Sprintf("%x", md5.Sum(buf.Bytes()))
	assert.Equal(t, expected, actual)
}

func TestInvalidFrameImage(t *testing.T) {
	frame, err := NewFrame()
	if err != nil {
		t.Error(err)
	}
	defer frame.Close()
	_, err = frame.ImageRGBA()

	assert.Equal(t, err, GoFFMPEGIO_ERROR_INVALID)
	assert.Equal(t, err.Error(), "GoFFMPEGIO_ERROR_INVALID")
}
