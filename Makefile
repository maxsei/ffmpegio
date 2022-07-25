PROG=ffmpeg-io

FFMPEG_LIBS= libavdevice   \
             libavformat   \
             libavfilter   \
             libavcodec    \
             libswresample \
             libswscale    \
             libavutil     \

CFLAGS += -Wall -g
# CFLAGS+=-Wall -std=c99 -g
CFLAGS := $(shell pkg-config --cflags $(FFMPEG_LIBS)) $(CFLAGS)

# LDFLAGS+=-lavcodec -lavformat
LDFLAGS := $(shell pkg-config --libs $(FFMPEG_LIBS)) $(LDLIBS)

all: ${PROG}

$(PROG): main.c
	gcc $(CFLAGS) main.c $(LDFLAGS) -o $(PROG)
clean:
	rm -f ${PROG}
