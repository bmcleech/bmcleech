#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "aspeed-xdma.h"

int aspeed_xdma_open() {
	return open("/dev/xdma", O_RDWR);
}

void *aspeed_xdma_mmap(int fd, size_t len, int prot) {
	return mmap(NULL, len, prot, MAP_SHARED, fd, /* offset= */ 0);
}

int aspeed_xdma_xfer(int fd, struct aspeed_xdma_op *op) {
	ssize_t res = write(fd, op, sizeof(*op));
	if (res == -1) return -1;
	if (res != sizeof(*op)) {
		// The kernel driver should only accept writes of sizeof(struct
		// aspeed_xdma_op) and read the whole struct at once.
		abort();
	}
	return 0;
}

int aspeed_xdma_read(int fd, uint64_t host_addr, int len) {
	struct aspeed_xdma_op op = {
		.upstream = 0,
		.host_addr = host_addr,
		.len = len,
	};
	return aspeed_xdma_xfer(fd, &op);
}

int aspeed_xdma_write(int fd, uint64_t host_addr, int len) {
	struct aspeed_xdma_op op = {
		.upstream = 1,
		.host_addr = host_addr,
		.len = len,
	};
	return aspeed_xdma_xfer(fd, &op);
}
