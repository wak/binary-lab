#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

void *map_file(const char *filename)
{
	int fd;
	size_t length;
	void *addr;
	struct stat st;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "open failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (fstat(fd, &st) < 0) {
		fprintf(stderr, "stat failed: %s\n", strerror(errno));
		goto err_close;
	}
	length = st.st_size;
	addr = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED) {
		fprintf(stderr, "mmap failed: %s, length=%lu\n",
			strerror(errno), length);
		goto err_close;
	}
	close(fd);
	return addr;

err_close:
	close(fd);
	exit(EXIT_FAILURE);
}

void die(const char *message)
{
	fprintf(stderr, "error: %s\n", message);
	exit(EXIT_FAILURE);
}

