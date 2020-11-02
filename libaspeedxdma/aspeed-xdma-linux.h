// include/uapi/linux/aspeed-xdma.h
// from https://lore.kernel.org/lkml/1558383565-11821-1-git-send-email-eajames@linux.ibm.com/

/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright IBM Corp 2019 */

#ifndef _UAPI_LINUX_ASPEED_XDMA_H_
#define _UAPI_LINUX_ASPEED_XDMA_H_

#include <linux/types.h>

/*
 * aspeed_xdma_op
 *
 * upstream: boolean indicating the direction of the DMA operation; upstream
 *           means a transfer from the BMC to the host
 *
 * host_addr: the DMA address on the host side, typically configured by PCI
 *            subsystem
 *
 * len: the size of the transfer in bytes; it should be a multiple of 16 bytes
 */
struct aspeed_xdma_op {
	__u32 upstream;
	__u64 host_addr;
	__u32 len;
};

#endif /* _UAPI_LINUX_ASPEED_XDMA_H_ */
