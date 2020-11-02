CC := arm-linux-gnueabi-gcc
CPPFLAGS := -I../libaspeedxdma
CFLAGS := -std=c11 -Wall -Wextra
LDFLAGS := -L../libaspeedxdma
LDLIBS := -laspeedxdma

.PHONY: all
all: getmem

.PHONY: clean
clean:
	$(RM) getmem

getmem: getmem.c
