FFMPEG_LIBS= libavdevice   \
             libavformat   \
             libavfilter   \
             libavcodec    \
             libswresample \
             libswscale    \
             libavutil     \

CFLAGS += -Wall -g -fPIC
CFLAGS := $(shell pkg-config --cflags $(FFMPEG_LIBS)) $(CFLAGS)
CFLAGS_EXAMPLES = -I./

LDFLAGS := $(shell pkg-config --libs $(FFMPEG_LIBS)) $(LDLIBS)
LDFLAGS_EXAMPLES = -l ffmpegio

export CGO_ENABLED=1
export CGO_LDFLAGS=$(LDFLAGS)
export CGO_CFLAGS=$(CFLAGS)

SOURCES_C=$(wildcard ./ffmpegio/*.c)
SOURCES_GO=$(wildcard ./ffmpegio/*.c)

all: test

example-framecounter: ./example/framecounter.c $(SOURCES_C)
	gcc $(CFLAGS) $(CFLAGS_EXAMPLES) ./example/framecounter.c $(LDFLAGS) $(LDFLAGS_EXAMPLES) -o ./bin/framecounter
	# gcc $(CFLAGS) $(CFLAGS_EXAMPLES) ./example/framecounter.c $(LDFLAGS) -l ./lib -o ./bin/framecounter
	# cd ./example; pwd; gcc $(CFLAGS) framecounter.c $(LDFLAGS) -o ../bin/framecounter

test: $(SOURCES_C) $(SOURCES_GO)
	go test ./ffmpegio
clean:
	# rm -f $(filter-out lib/.gitkeep, $(wildcard lib/*))
	rm -f $(filter-out lib/.gitkeep, $(wildcard lib/*))
