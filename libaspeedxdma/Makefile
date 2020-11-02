CC := arm-linux-gnueabi-gcc
CFLAGS := -std=c11 -Wall -Wextra

.PHONY: all
all: libaspeedxdma.a

.PHONY: clean
clean:
	$(RM) aspeed-xdma.o
	$(RM) libaspeedxdma.a

aspeed-xdma.o: aspeed-xdma.c aspeed-xdma.h aspeed-xdma-linux.h

libaspeedxdma.a: aspeed-xdma.o
	ar rcs $@ $^
