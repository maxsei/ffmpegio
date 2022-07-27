FFMPEG_LIBS= libavdevice   \
             libavformat   \
             libavfilter   \
             libavcodec    \
             libswresample \
             libswscale    \
             libavutil     \

CFLAGS += -Wall -g -fPIC
CFLAGS := $(shell pkg-config --cflags $(FFMPEG_LIBS)) $(CFLAGS)

LDFLAGS := $(shell pkg-config --libs $(FFMPEG_LIBS))

export CGO_ENABLED=1
export CGO_LDFLAGS=$(LDFLAGS)
export CGO_CFLAGS=$(CFLAGS)

SOURCES_C=$(wildcard ./ffmpegio/*.c)
SOURCES_GO=$(wildcard ./ffmpegio/*.c)

all: test

framecounterc: ./example/framecounter.c $(SOURCES_C)
	gcc $(CFLAGS) -I./ $^ $(LDFLAGS) -o ./bin/framecounterc

	# TODO: figure out how to statically link this.
	# gcc $(CFLAGS) -I./ $^ -static $(LDFLAGS) -o ./bin/framecounter-static

test: $(SOURCES_C) $(SOURCES_GO)
	go test -v ./ffmpegio
clean:
	rm -f $(filter-out bin/.gitkeep, $(wildcard bin/*))
