#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(
	int argc,
	char ** argv)
{
	if (4 != argc)
	{
		fprintf(stderr, "usage: %s raw_rgb_file dim_x dim_y\n", argv[0]);
		return -1;
	}

	unsigned dim_x, dim_y;

	sscanf(argv[2], "%u", &dim_x);
	sscanf(argv[3], "%u", &dim_y);

	if (!dim_x || !dim_y)
	{
		fprintf(stderr, "nil dimension specified; abort\n");
		return -2;
	}

	FILE* f = fopen(argv[1], "r+b");

	if (!f)
	{
		fprintf(stderr, "failed to open %s; abort\n", argv[1]);
		return -3;
	}

	const unsigned sizeofpix = sizeof(uint8_t[3]);
	const unsigned sizeofbuf = dim_x * dim_y * sizeofpix + 1;

	uint8_t* buffer = (uint8_t*) malloc(sizeofbuf);

	if (!buffer)
	{
		fclose(f);
		free(buffer);
		fprintf(stderr, "failed to allocate buffer memory of %u bytes; abort\n", sizeofbuf);
		return -4;
	}

	const size_t n_read = fread(buffer, 1, sizeofbuf, f);

	if (dim_x * dim_y * sizeofpix != n_read)
	{
		fclose(f);
		free(buffer);
		fprintf(stderr, "read unexpected number of bytes (expected: %u, read: %u); abort\n", dim_x * dim_y * sizeofpix, n_read);
		return -5;
	}

	// touch bitmap here
	for (unsigned i = 0; i < dim_x * dim_y * sizeofpix; i += 3)
		buffer[i + 1] ^= 0xff;

	fseek(f, 0L, SEEK_SET);
	const size_t n_write = fwrite(buffer, sizeofpix, dim_x * dim_y, f);

	fclose(f);
	free(buffer);

	if (dim_x * dim_y != n_write)
	{
		fprintf(stderr, "wrote unexpected number of pixels (expected: %u, wrote: %u); abort\n", dim_x * dim_y, n_write);
		return -6;
	}

	return 0;
}
