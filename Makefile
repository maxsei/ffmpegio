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

EXAMPLE_C=$(wildcard ./example/*.c)
SOURCES_C=$(wildcard ./ffmpegio/*.c)
SOURCES_GO=$(wildcard ./ffmpegio/*.go)

all: framecounterc mainc test

framecounterc: ./example/framecounter.c $(SOURCES_C)
	gcc $(CFLAGS) -I./ $^ $(LDFLAGS) -o ./bin/framecounterc

	# TODO: figure out how to statically link this.
	# gcc $(CFLAGS) -I./ $^ -static $(LDFLAGS) -o ./bin/framecounter-static

mainc: ./example/main.c $(SOURCES_C)
	gcc $(CFLAGS) -I./ $^ $(LDFLAGS) -o ./bin/mainc

test: $(SOURCES_C) $(SOURCES_GO)
	go test -v ./ffmpegio
fmt: $(SOURCES_C) $(SOURCES_GO) $(EXAMPLE_C)
	clang-format -i --style=google $(EXAMPLE_C) $(SOURCES_C)
	go fmt $(SOURCES_GO)
clean:
	rm -f $(filter-out bin/.gitkeep, $(wildcard bin/*))
