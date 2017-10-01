#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define RAND_SRC "/dev/urandom"

int main(void)
{
	int random_fd = open(RAND_SRC, O_RDONLY);
	char result;

	if (random_fd < 0) {
		fprintf(stderr, "Could not open random file, " RAND_SRC "\n");
		return -1;
	}

	if (read(random_fd, &result, sizeof(result)) < sizeof(result)) {
		fprintf(stderr, "Failed to read random byte.\n");
	} else {
		if ((result & 1) == 0) {
			printf("Even\n");
		} else {
			printf("Odd\n");
		}
	}

	if (close(random_fd) < 0) {
		fprintf(stderr, "Could not open close file, " RAND_SRC "\n");
	}

	return 0;
}
