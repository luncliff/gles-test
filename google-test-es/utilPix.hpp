#ifndef util_pix_H__
#define util_pix_H__

#include <stdint.h>

namespace testbed
{

namespace util
{

struct pix
{
	uint8_t c[3];

	pix() {}

	pix(const uint8_t r,
		const uint8_t g,
		const uint8_t b)
	{
		c[0] = r;
		c[1] = g;
		c[2] = b;
	}
};


inline unsigned
next_multiple_of_pix_integral(
	const unsigned unaligned_size)
{
	// compute pix's optimal integral size
	unsigned integral_size = sizeof(pix);

	// isolate the leading bit
	while (integral_size & integral_size - 1)
		integral_size &= integral_size - 1;

	// double up if original quantity was not power of two
	if (sizeof(pix) & sizeof(pix) - 1)
		integral_size <<= 1;

	return (unaligned_size + integral_size - 1) & ~(integral_size - 1);
}


void
fill_with_checker(
	pix* const buffer,
	const unsigned stride,
	const unsigned dim_x,
	const unsigned dim_y,
	const unsigned granularity = 8);

bool
fill_from_file(
	pix* const buffer,
	const unsigned stride,
	const unsigned dim_x,
	const unsigned dim_y,
	const char* const filename);

void
fill_YUV420_from_RGB(
	uint8_t* const y_buffer,
	uint8_t* const u_buffer,
	uint8_t* const v_buffer,
	const unsigned y_stride,
	const unsigned u_stride,
	const unsigned v_stride,
	pix* const rgb_buffer,
	const unsigned rgb_stride,
	const unsigned dim_x,
	const unsigned dim_y);

} // namespace hook
} // namespace testbed

#endif // util_pix_H__
