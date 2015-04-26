SHELL = /bin/sh
CC    = gcc

FLAGS        = -I /home/nastasi/git/ffmpeg -I /home/nastasi/ffmpeg/include # -std=gnu99 -Iinclude
CFLAGS       = -Wall -fPIC # -g -fPIC -pedantic -Wall -Wextra -ggdb3
LDFLAGS      = -shared -lavfilter -lgd -lpng -lz -ljpeg -lfreetype -lm

DEBUGFLAGS   = -g -O0 -D _DEBUG
#RELEASEFLAGS = -O2 -D NDEBUG -combine -fwhole-program

TARGET  = plugin/vf_chronograph.so
SOURCES = vf_chronograph.c
HEADERS = $(shell echo include/*.h)
OBJECTS = $(SOURCES:.c=.o)

# PREFIX = $(DESTDIR)/usr/local
# BINDIR = $(PREFIX)/bin

all: $(TARGET)

%.o: %.c
	$(CC) $(FLAGS) $(CFLAGS) $(DEBUGFLAGS) -c $?

$(TARGET): $(OBJECTS)
	$(CC) $(FLAGS) -o $(TARGET) $(CFLAGS) $(OBJECTS) $(LDFLAGS) $(DEBUGFLAGS)

clean:
	rm -f *.o plugin/*.so out/*.mpg core

.PHONY: all clean
