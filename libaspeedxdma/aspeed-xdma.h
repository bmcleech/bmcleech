#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/mman.h>

// For struct aspeed_xdma_op, should be replaced with the following once the
// patch is merged to the upstream kernel:
// #include <linux/aspeed-xdma.h>
#include "aspeed-xdma-linux.h"

int aspeed_xdma_open();
void *aspeed_xdma_mmap(int fd, size_t len, int prot);
int aspeed_xdma_xfer(int fd, struct aspeed_xdma_op *op);
int aspeed_xdma_read(int fd, uint64_t host_addr, int len);
int aspeed_xdma_write(int fd, uint64_t host_addr, int len);

#ifdef __cplusplus
}
#endif
