#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <unistd.h>

#include <aspeed-xdma.h>

const size_t MAX_REQUEST_BYTES = 8 * 1024 * 1024; // 8 MiB
const size_t MAX_TOTAL_REQUEST_BYTES = 32 * 1024 * 1024; // 32 MiB
static int xdma_fd = -1;
static void *xdma_mem = NULL;

enum rawtcp_cmd {
	STATUS,
	MEM_READ,
	MEM_WRITE,
	_FORCE_SIZE = UINT64_MAX,
};
_Static_assert(sizeof(enum rawtcp_cmd) == sizeof(uint64_t), "cmd must be 64 bit in size (quad word)");

struct rawtcp_msg {
	enum rawtcp_cmd cmd;
	uint64_t addr;
	uint64_t cb;
};
_Static_assert(sizeof(struct rawtcp_msg) == 3 * sizeof(uint64_t), "req must be 3 quad words long");

static int recv_rawtcp_req(int fd, struct rawtcp_msg *req) {
	ssize_t len = recv(fd, req, sizeof(*req), MSG_WAITALL);
	if (len > 0) assert(len == sizeof(*req));
#if __BYTE_ORDER != __LITTLE_ENDIAN
/* The RawTCP device implementation in pcileech seems to use the endianness of
 * the machine it is running on, typically little-endian on x86/x86_64. This is
 * usually fine as this code is intended to on a AST2500 little-endian ARM. If
 * this is not the case, some byte-swapping would be needed here. */
#error "expected little endian machine, see comment in the source code"
#endif
	return len > 0 ? 1 : len;
}

static int send_rawtcp_resp(int fd, const struct rawtcp_msg *req, const void *data, size_t len) {
	ssize_t ret;
	struct rawtcp_msg resp = {
		.cmd = req->cmd,
		.addr = req->addr,
		.cb = len,
	};
	ret = send(fd, &resp, sizeof(resp), MSG_NOSIGNAL);
	if (ret == -1) {
		return -1;
	}
	assert((size_t) ret == sizeof(resp));
	if (len > 0) {
		ret = send(fd, data, len, MSG_NOSIGNAL);
		if (ret == -1) {
			return -1;
		}
		assert((size_t) ret == len);
	}
	return 0;
}

static int print_rawtcp_req(FILE *f, const struct rawtcp_msg *req) {
	const char *cmd_str = "unknown";
	if (req->cmd == STATUS) cmd_str = "STATUS";
	if (req->cmd == MEM_READ) cmd_str = "MEM_READ";
	if (req->cmd == MEM_WRITE) cmd_str = "MEM_WRITE";
	return fprintf(f, "struct rawtcp_msg{.cmd=%s(%" PRIu64 "), .addr=%016" PRIx64 ", .cb=%016" PRIx64 "}",
			cmd_str, req->cmd, req->addr, req->cb);
}

static int handle_status_req(int client_fd, const struct rawtcp_msg *req) {
	fprintf(stderr, "Handling status request\n");
	uint8_t resp = 1;
	return send_rawtcp_resp(client_fd, req, &resp, sizeof(resp));
}

static int handle_mem_read_req(int client_fd, const struct rawtcp_msg *req) {
	int result = -1;

	fprintf(stderr, "Handling memory read request\n");
	if (req->cb > MAX_TOTAL_REQUEST_BYTES) {
		fprintf(stderr, "requested number of bytes is too large: %" PRIu64 " (max: %zu)\n", req->cb, MAX_TOTAL_REQUEST_BYTES);
		errno = EFBIG;
		return -1;
	}
	fprintf(stderr, "Read %" PRIu64 " bytes\n", req->cb);

	uint8_t *mem = xdma_mem;
	if (req->cb > MAX_REQUEST_BYTES) {
		fprintf(stderr, "requested number of bytes is too large for a single DMA transfer, using slow code path: %" PRIu64 " (max: %zu)\n", req->cb, MAX_REQUEST_BYTES);
		mem = malloc(req->cb);
		if (mem == NULL) {
			result = -1;
			goto out;
		}
	}

	for (uint64_t off = 0; off < req->cb; off += MAX_REQUEST_BYTES) {
		uint64_t xfer_addr = req->addr + off;
		uint64_t xfer_size = MIN(MAX_REQUEST_BYTES, req->cb - off);
		if (xfer_addr < req->addr) {
			errno = EINVAL;
			result = -1;
			goto out;
		}
		fprintf(stderr, "DMA read %" PRIu64 " bytes at %" PRIx64 "\n", xfer_size, xfer_addr);
		if (aspeed_xdma_read(xdma_fd, xfer_addr, xfer_size)) {
			perror("aspeed_xdma_read()");
			result = -1;
			goto out;
		}
		if (mem != xdma_mem) {
			memcpy(mem + off, xdma_mem, xfer_size);
		}
	}

	result = send_rawtcp_resp(client_fd, req, mem, req->cb);
out:
	if (mem != xdma_mem) {
		free(mem);
	}
	return result;
}

static int handle_mem_write_req(int client_fd, const struct rawtcp_msg *req) {
	fprintf(stderr, "Handling memory write request\n");
	if (req->cb > MAX_REQUEST_BYTES) {
		fprintf(stderr, "requested number of bytes is too large: %" PRIu64 " (max: %zu)\n", req->cb, MAX_REQUEST_BYTES);
		errno = EFBIG;
		return -1;
	}
	fprintf(stderr, "Write %" PRIu64 " bytes\n", req->cb);
	ssize_t len = recv(client_fd, xdma_mem, req->cb, MSG_WAITALL);
	if (len == 0) {
		fprintf(stderr, "Client closed the connection\n");
		errno = EIO;
		return -1;
	} else if (len == -1) {
		return -1;
	} else {
		assert((size_t) len == req->cb);
	}
	if (aspeed_xdma_write(xdma_fd, req->addr, req->cb) < 0) {
		perror("aspeed_xdma_write()");
		return -1;
	}
	return send_rawtcp_resp(client_fd, req, NULL, 0);
}

static int handle_rawtcp_req(int client_fd, const struct rawtcp_msg *req) {
	switch (req->cmd) {
		case STATUS:
			return handle_status_req(client_fd, req);

		case MEM_READ:
			return handle_mem_read_req(client_fd, req);

		case MEM_WRITE:
			return handle_mem_write_req(client_fd, req);

		default:
			fprintf(stderr, "Received invalid or unkown request command %" PRIu64 "\n", req->cmd);
			errno = EBADRQC;
			return -1;
	}
}

static void usage(const char *argv0) {
	fprintf(stderr, "usage: %s port\n", argv0);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		usage(argv[0]);
	}

	xdma_fd = aspeed_xdma_open();
	if (xdma_fd < 0) {
		perror("aspeed_xdma_open()");
		return EXIT_FAILURE;
	}

	xdma_mem = aspeed_xdma_mmap(xdma_fd, MAX_REQUEST_BYTES, PROT_READ|PROT_WRITE);
	if (xdma_mem == MAP_FAILED) {
		perror("aspeed_xdma_mmap()");
		return EXIT_FAILURE;
	}

	int sock_fd = socket(AF_INET6, SOCK_STREAM, 0);
	if (sock_fd == -1) {
		perror("socket()");
		return EXIT_FAILURE;
	}

	struct sockaddr_in6 sock_addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = { /* [::] (any) */ },
		.sin6_port = htons(atoi(argv[1])),
	};
	if (bind(sock_fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) == -1) {
		perror("bind()");
		return EXIT_FAILURE;
	}

	if (listen(sock_fd, SOMAXCONN) == -1) {
		perror("listen()");
		return EXIT_FAILURE;
	}
	char sock_addr_str[INET6_ADDRSTRLEN] = "";
	fprintf(stderr, "Listening on [%s]:%d\n",
			inet_ntop(AF_INET6, &sock_addr.sin6_addr, sock_addr_str, sizeof(sock_addr_str)),
			ntohs(sock_addr.sin6_port));

	while (true) {
		struct sockaddr_in6 client_addr = {};
		socklen_t client_addr_len = sizeof(client_addr);
		fprintf(stderr, "Waiting to accept connection\n");
		int client_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &client_addr_len);
		if (client_fd == -1) {
			perror("accept()");
			return EXIT_FAILURE;
		} else if (client_addr_len != sizeof(client_addr)) {
			fprintf(stderr, "accept() returned an unexpected address length: got %u, expected %zu (sa_family=%d)\n",
					client_addr_len, sizeof(client_addr), client_addr.sin6_family);
			return EXIT_FAILURE;
		} else if (client_addr.sin6_family != AF_INET6) {
			fprintf(stderr, "accept() returned an unexpected address family: got %d, expected %d\n",
					client_addr.sin6_family, AF_INET6);
			return EXIT_FAILURE;
		}

		char client_addr_str[INET6_ADDRSTRLEN] = "";
		fprintf(stderr, "Accepted connection from [%s]:%d\n",
				inet_ntop(AF_INET6, &client_addr.sin6_addr, client_addr_str, sizeof(client_addr_str)),
				ntohs(client_addr.sin6_port));

		while (true) {
			fprintf(stderr, "Waiting for request\n");
			struct rawtcp_msg req;
			int res = recv_rawtcp_req(client_fd, &req);
			if (res <= 0) {
				if (res == 0) {
					fprintf(stderr, "Client closed the connection\n");
				} else {
					perror("recv()");
				}
				break;
			}
			fprintf(stderr, "Received request: ");
			print_rawtcp_req(stderr, &req);
			fprintf(stderr, "\n");

			if (handle_rawtcp_req(client_fd, &req) == -1) {
				perror("handle request");
				break;
			}
		}

		fprintf(stderr, "Closing connection\n");
		close(client_fd);
	}

	close(sock_fd);
	return EXIT_SUCCESS;
}
