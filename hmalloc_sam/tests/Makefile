
SRCS := $(wildcard *.c)
BINS := $(SRCS:.c=)
CFLAGS := -g -std=gnu99 -I..

all: $(BINS)

$(BINS): $(SRCS)
	gcc $(CFLAGS) -o $@ $@.c ../hmalloc.c ../hmalloc.h

clean:
	rm -f $(BINS)
