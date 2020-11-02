#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <aspeed-xdma.h>

#define HOST_PAGE_SIZE (4096)

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "usage: %s <host-addr>\n", argv[0]);
		return EXIT_FAILURE;
	}

	errno = 0;
	char *end;
	unsigned long long host_addr = host_addr = strtoull(argv[1], &end, 0);
	if ((host_addr == ULLONG_MAX && errno == ERANGE) || *end != '\0') {
		fprintf(stderr, "invalid host address\n");
		return EXIT_FAILURE;
	}

	if (isatty(STDOUT_FILENO)) {
		fprintf(stderr, "refusing to send binary data to stdout\n");
		return EXIT_FAILURE;
	}

	int xdma = aspeed_xdma_open();
	if (xdma < 0) {
		perror("aspeed_xdma_open()");
		return EXIT_FAILURE;
	}

	void *buf = aspeed_xdma_mmap(xdma, HOST_PAGE_SIZE, PROT_READ);
	if (buf == MAP_FAILED) {
		perror("aspeed_xdma_mmap()");
		return EXIT_FAILURE;
	}

	if (aspeed_xdma_read(xdma, host_addr, HOST_PAGE_SIZE) < 0) {
		perror("aspeed_xdma_read()");
		return EXIT_FAILURE;
	}

	fwrite(buf, HOST_PAGE_SIZE, 1, stdout);
}
