CC := arm-linux-gnueabi-gcc
CPPFLAGS := -I../libaspeedxdma
CFLAGS := -std=c11 -Wall -Wextra
LDFLAGS := -L../libaspeedxdma
LDLIBS := -laspeedxdma

.PHONY: all
all: xdma-rawtcpd

.PHONY: clean
clean:
	$(RM) xdma-rawtcpd

xdma-rawtcpd: xdma-rawtcpd.c
