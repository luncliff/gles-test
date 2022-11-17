#ifndef testbed_H__
#define testbed_H__

#if defined(PLATFORM_GLX)

#include <GL/gl.h>

#else

#include <GLES2/gl2.h>

#endif

#include <iostream>
#include "pure_macro.hpp"
#include "scoped.hpp"

#define DEBUG_GL_ERR()												\
	if (DEBUG_LITERAL && testbed::util::reportGLError())			\
	{																\
		std::cerr << __FILE__ ":" XQUOTE(__LINE__) << std::endl;	\
		return false;												\
	}

namespace testbed
{

extern const char arg_prefix[];
extern const char arg_app[];

namespace util
{

bool
setupShader(
	const GLuint shader_name,
	const char* const filename);

bool
setupShaderWithPatch(
	const GLuint shader_name,
	const char* const filename,
	const size_t patch_count,
	const std::string* const patch);

bool
setupProgram(
	const GLuint prog_name,
	const GLuint shader_v_name,
	const GLuint shader_f_name);

bool
reportGLError(
	std::ostream& stream = std::cerr);

#if !defined(PLATFORM_GLX)

bool
reportEGLError(
	std::ostream& stream = std::cerr);

#endif

uintptr_t
getNativeWindowSystem();

} // namespace util

namespace hook
{

bool
init_resources(
	const unsigned argc,
	const char* const * argv);

bool
deinit_resources();

bool
render_frame();

bool
input_frame(
	const GLuint input);

enum {
	INPUT_MASK_ESC			= 1 << 0,
	INPUT_MASK_LEFT			= 1 << 1,
	INPUT_MASK_RIGHT		= 1 << 2,
	INPUT_MASK_DOWN			= 1 << 3,
	INPUT_MASK_UP			= 1 << 4,

	INPUT_MASK_FORCE_TYPE	= GLuint(-1)
};

bool
set_num_drawcalls(
	const unsigned n);

unsigned
get_num_drawcalls();

bool
requires_depth();

} // namespace hook
} // namespace testbed

#endif // testbed_H__
