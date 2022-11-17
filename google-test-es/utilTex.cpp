#include <stdlib.h>
#include <assert.h>
#include <iostream>

#include "testbed.hpp"
#include "utilPix.hpp"
#include "utilTex.hpp"

#if defined(PLATFORM_GLX)
#if defined(GL_LUMINANCE)
#undef GL_LUMINANCE
#endif
#define GL_LUMINANCE GL_RED
#endif

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

namespace util
{

bool
setupTexture2D(
	const GLuint tex_name,
	const char* filename,
	const unsigned tex_w,
	const unsigned tex_h)
{
	assert(0 != tex_name);
	assert(0 != filename);

	const unsigned pix_size = sizeof(pix);
	const unsigned tex_size = tex_h * tex_w * pix_size;

	// provide some guardband as pixels are of non-word-multiple size
	scoped_ptr< pix, generic_free > tex_src(
		reinterpret_cast< pix* >(malloc(next_multiple_of_pix_integral(tex_size))));

	if (0 == tex_src())
	{
		std::cerr << __FUNCTION__ <<
			" failed to allocate memory for texture '" << filename << "'" << std::endl;
		return false;
	}

	if (fill_from_file(tex_src(), tex_w * pix_size, tex_w, tex_h, filename))
	{
		std::cout << "texture bitmap '" << filename << "' ";
	}
	else
	{
		fill_with_checker(tex_src(), tex_w * pix_size, tex_w, tex_h);

		std::cout << "checker texture ";
	}

	std::cout << tex_w << " x " << tex_h << " x " << (pix_size * 8) << " bpp, " <<
		tex_size << " bytes" << std::endl;

	const bool pot = !(tex_w & tex_w - 1) && !(tex_h & tex_h - 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_name);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pot ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_w, tex_h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_RGB, GL_UNSIGNED_BYTE, tex_src());

	if (pot)
	{
		std::cout << "expanded into a mipmap" << std::endl;

		glGenerateMipmap(GL_TEXTURE_2D);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	const bool success = !reportGLError();

	if (!success)
		std::cerr << __FUNCTION__ << " failed to setup a texture" << std::endl;

	return success;
}


bool
setupTextureYUV420(
	const GLuint (&tex_name)[3],
	const char* filename,
	const unsigned tex_w,
	const unsigned tex_h)
{
	for (unsigned i = 0; i < sizeof(tex_name) / sizeof(tex_name[0]); ++i)
		assert(0 != tex_name[i]);

	assert(0 != filename);

	const unsigned pix_size = sizeof(pix);
	const unsigned tex_size = tex_h * tex_w * pix_size;

	// provide some guardband as pixels are of non-word-multiple size
	scoped_ptr< pix, generic_free > tex_src(
		reinterpret_cast< pix* >(malloc(next_multiple_of_pix_integral(tex_size))));

	if (0 == tex_src())
	{
		std::cerr << __FUNCTION__ <<
			" failed to allocate memory for texture '" << filename << "'" << std::endl;
		return false;
	}

	if (fill_from_file(tex_src(), tex_w * pix_size, tex_w, tex_h, filename))
	{
		std::cout << "texture bitmap '" << filename << "' ";
	}
	else
	{
		fill_with_checker(tex_src(), tex_w * pix_size, tex_w, tex_h);

		std::cout << "checker texture ";
	}

	std::cout << tex_w << " x " << tex_h << " x " << (pix_size * 8) << " bpp, " <<
		tex_size << " bytes" << std::endl;

	scoped_ptr< uint8_t, generic_free > yuv_tex_src(
		reinterpret_cast< uint8_t* >(malloc(tex_w * tex_h * 3 / 2)));

	if (0 == yuv_tex_src())
	{
		std::cerr << __FUNCTION__ <<
			" failed to allocate memory for texture '" << filename << "'" << std::endl;
		return false;
	}

	uint8_t* y_tex_src = yuv_tex_src();
	uint8_t* u_tex_src = yuv_tex_src() + tex_w * tex_h;
	uint8_t* v_tex_src = yuv_tex_src() + tex_w * tex_h * 5 / 4;

	fill_YUV420_from_RGB(
		y_tex_src,
		u_tex_src,
		v_tex_src,
		tex_w, tex_w / 2, tex_w / 2,
		tex_src(),
		tex_w * pix_size,
		tex_w, tex_h);

	const bool pot = !(tex_w & tex_w - 1) && !(tex_h & tex_h - 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_name[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pot ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, tex_w, tex_h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_LUMINANCE, GL_UNSIGNED_BYTE, y_tex_src);

	if (pot)
	{
		std::cout << "expanded into a mipmap" << std::endl;

		glGenerateMipmap(GL_TEXTURE_2D);
	}

	glBindTexture(GL_TEXTURE_2D, tex_name[1]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pot ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, tex_w / 2, tex_h / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w / 2, tex_h / 2, GL_LUMINANCE, GL_UNSIGNED_BYTE, u_tex_src);

	if (pot)
	{
		std::cout << "expanded into a mipmap" << std::endl;

		glGenerateMipmap(GL_TEXTURE_2D);
	}

	glBindTexture(GL_TEXTURE_2D, tex_name[2]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pot ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, tex_w / 2, tex_h / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w / 2, tex_h / 2, GL_LUMINANCE, GL_UNSIGNED_BYTE, v_tex_src);

	if (pot)
	{
		std::cout << "expanded into a mipmap" << std::endl;

		glGenerateMipmap(GL_TEXTURE_2D);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	const bool success = !reportGLError();

	if (!success)
		std::cerr << __FUNCTION__ << " failed to setup a texture" << std::endl;

	return success;
}


bool
setupTexture2D(
	const GLuint tex_name,
	const pix* tex_src,
	const unsigned tex_w,
	const unsigned tex_h)
{
	assert(0 != tex_name);
	assert(0 != tex_src);

	const unsigned pix_size = sizeof(pix);
	const unsigned tex_size = tex_h * tex_w * pix_size;

	std::cout << "client texture " << tex_src << " " <<
		tex_w << " x " << tex_h << " x " << (pix_size * 8) << " bpp, " << tex_size << " bytes" << std::endl;

	const bool pot = !(tex_w & tex_w - 1) && !(tex_h & tex_h - 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_name);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pot ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_w, tex_h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_RGB, GL_UNSIGNED_BYTE, tex_src);

	if (pot)
	{
		std::cout << "expanded into a mipmap" << std::endl;

		glGenerateMipmap(GL_TEXTURE_2D);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	const bool success = !reportGLError();

	if (!success)
		std::cerr << __FUNCTION__ << " failed to setup a texture" << std::endl;

	return success;
}

} // namespace util
} // namespace testbed
