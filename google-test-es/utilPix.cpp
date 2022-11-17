#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "get_file_size.hpp"
#include "testbed.hpp"
#include "utilPix.hpp"

namespace testbed
{

template < typename T >
class generic_free
{
public:

	void operator()(T* arg)
	{
		free(arg);
	}
};


template <>
class scoped_functor< FILE >
{
public:

	void operator()(FILE* arg)
	{
		fclose(arg);
	}
};

namespace util
{

void
fill_with_checker(
	pix* const buffer,
	const unsigned stride,
	const unsigned dim_x,
	const unsigned dim_y,
	const unsigned granularity)
{
	const pix pixA(255U, 128U, 128U);
	const pix pixB(128U, 128U, 255U);

	for (unsigned y = 0; y < dim_y; ++y)
		for (unsigned x = 0; x < dim_x; ++x)
		{
			*(reinterpret_cast< pix* >(reinterpret_cast< uint8_t* >(buffer) + y * stride) + x) =
				(y & granularity) ^ (x & granularity)
					? pixB
					: pixA;
		}
}


bool
fill_from_file(
	pix* const buffer,
	const unsigned stride,
	const unsigned dim_x,
	const unsigned dim_y,
	const char* const filename)
{
	size_t len;

	if (!get_file_size(filename, len))
	{
		std::cerr << __FUNCTION__ << " failed getting size of file '" << filename << "'" << std::endl;
		return false;
	}

	const unsigned pix_size = sizeof(pix);
	const unsigned row_size = dim_x * pix_size;
	const unsigned img_size = dim_y * row_size;

	const unsigned optional_header_size = 8;

	if (len != img_size &&
		len != img_size + optional_header_size)
	{
		std::cerr << __FUNCTION__ << " got unexpected size for file '" << filename << "'" << std::endl;
		return false;
	}

	scoped_ptr< FILE, scoped_functor > file(fopen(filename, "rb"));

	if (0 == file())
	{
		std::cerr << __FUNCTION__ << " failed to open file '" << filename << "'" << std::endl;
		return false;
	}

	if (len == img_size + optional_header_size)
	{
		// skip optional header
		fseek(file(), optional_header_size, SEEK_SET);
	}

	unsigned read = 0;

	if (row_size != stride)
	{
		scoped_ptr< pix, generic_free > tmp(
			reinterpret_cast< pix* >(malloc(img_size)));

		read = fread(tmp(), img_size, 1, file());

		for (unsigned i = 0; i < dim_y; ++i)
			memcpy(reinterpret_cast< uint8_t* >(buffer) + i * stride,
				tmp() + i * dim_x, row_size);
	}
	else
		read = fread(buffer, img_size, 1, file());

	if (1 != read)
	{
		std::cerr << __FUNCTION__ << " failed to read file '" << filename << "'" << std::endl;
		return false;
	}

	return true;
}


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
	const unsigned dim_y)
{
	const pix* rgb_src[2] =
	{
		rgb_buffer,
		reinterpret_cast< pix* >(
			reinterpret_cast< uint8_t* >(rgb_buffer) + rgb_stride)
	};

	uint8_t* y_dst[2] =
	{
		y_buffer,
		y_buffer + y_stride
	};

	uint8_t* u_dst = u_buffer;
	uint8_t* v_dst = v_buffer;

	const unsigned rgb_pad = rgb_stride - dim_x * sizeof(pix);
	const unsigned y_pad = y_stride - dim_x;
	const unsigned u_pad = u_stride - dim_x / 2;
	const unsigned v_pad = v_stride - dim_x / 2;

	for (unsigned i = 0; i < dim_y / 2; ++i)
	{
		for (unsigned j = 0; j < dim_x / 2; ++j)
		{
			const float rgb[4][3] =
			{
				{
					rgb_src[0][0].c[0] / 255.f,
					rgb_src[0][0].c[1] / 255.f,
					rgb_src[0][0].c[2] / 255.f
				},
				{
					rgb_src[0][1].c[0] / 255.f,
					rgb_src[0][1].c[1] / 255.f,
					rgb_src[0][1].c[2] / 255.f
				},
				{
					rgb_src[1][0].c[0] / 255.f,
					rgb_src[1][0].c[1] / 255.f,
					rgb_src[1][0].c[2] / 255.f
				},
				{
					rgb_src[1][1].c[0] / 255.f,
					rgb_src[1][1].c[1] / 255.f,
					rgb_src[1][1].c[2] / 255.f
				}
			};

			const float rgb_filt[3] =
			{
				(rgb[0][0] + rgb[1][0] + rgb[2][0] + rgb[3][0]) * .25f,
				(rgb[0][1] + rgb[1][1] + rgb[2][1] + rgb[3][1]) * .25f,
				(rgb[0][2] + rgb[1][2] + rgb[2][2] + rgb[3][2]) * .25f
			};

			// ITU-R BT.601 standard RGB-to-YPbPr
			const float f[3][4] =
			{
				{  .299f,  .587f,  .114f,   0.f  },
				{ -.169f, -.331f,  .499f,   .5f  },
				{  .499f, -.418f, -.0813f,  .5f  }
			};

			const float y[4] =
			{
				f[0][0] * rgb[0][0] + f[0][1] * rgb[0][1] + f[0][2] * rgb[0][2],
				f[0][0] * rgb[1][0] + f[0][1] * rgb[1][1] + f[0][2] * rgb[1][2],
				f[0][0] * rgb[2][0] + f[0][1] * rgb[2][1] + f[0][2] * rgb[2][2],
				f[0][0] * rgb[3][0] + f[0][1] * rgb[3][1] + f[0][2] * rgb[3][2]
			};

			const float u =
				f[1][0] * rgb_filt[0] + f[1][1] * rgb_filt[1] + f[1][2] * rgb_filt[2] + f[1][3];

			const float v =
				f[2][0] * rgb_filt[0] + f[2][1] * rgb_filt[1] + f[2][2] * rgb_filt[2] + f[2][3];

			y_dst[0][0] = uint8_t(y[0] * 255.f);
			y_dst[0][1] = uint8_t(y[1] * 255.f);
			y_dst[1][0] = uint8_t(y[2] * 255.f);
			y_dst[1][1] = uint8_t(y[3] * 255.f);

			u_dst[0] = uint8_t(u * 255.f);
			v_dst[0] = uint8_t(v * 255.f);

			rgb_src[0] += 2;
			rgb_src[1] += 2;

			y_dst[0] += 2;
			y_dst[1] += 2;
			u_dst += 1;
			v_dst += 1;
		}

		rgb_src[0] = reinterpret_cast< const pix* >(
			reinterpret_cast< const uint8_t* >(rgb_src[1]) + rgb_pad);
		rgb_src[1] = reinterpret_cast< const pix* >(
			reinterpret_cast< const uint8_t* >(rgb_src[0]) + rgb_stride);

		y_dst[0] = y_dst[1] + y_pad;
		y_dst[1] = y_dst[0] + y_stride;

		u_dst += u_pad;
		v_dst += v_pad;
	}
}

} // namespace util
} // namespace testbed
