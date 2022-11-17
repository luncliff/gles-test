#ifndef rend_indexed_trilist_H__
#define rend_indexed_trilist_H__

#if defined(PLATFORM_GLX)

#include <GL/gl.h>

#else

#include <GLES2/gl2.h>

#endif

namespace testbed
{

namespace util
{

bool
fill_indexed_trilist_from_file_PN(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	unsigned& num_faces,
	GLenum& index_type,
	const bool is_rotated);

bool
fill_indexed_trilist_from_file_PN2(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	unsigned& num_faces,
	GLenum& index_type,
	const bool is_rotated);

bool
fill_indexed_trilist_from_file_AGE(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	const uintptr_t (&semantics_offset)[4],
	unsigned& num_faces,
	GLenum& index_type,
	float (&bmin)[3],
	float (&bmax)[3]);

} // namespace util
} // namespace testbed

#endif // rend_indexed_trilist_H__
