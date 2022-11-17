#ifndef util_tex_H__
#define util_tex_H__

#if defined(PLATFORM_GLX)

#include <GL/gl.h>

#else

#include <GLES2/gl2.h>

#endif

#include "utilPix.hpp"

namespace testbed
{

namespace util
{

bool
setupTexture2D(
	const GLuint tex_name,
	const char* filename,
	const unsigned tex_w,
	const unsigned tex_h);

bool
setupTextureYUV420(
	const GLuint (&tex_name)[3],
	const char* filename,
	const unsigned tex_w,
	const unsigned tex_h);

bool
setupTexture2D(
	const GLuint tex_name,
	const pix* tex_src,
	const unsigned tex_w,
	const unsigned tex_h);

} // namespace util
} // namespace testbed

#endif // util_tex_H__
