#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#ifdef USE_XRANDR
#include "xrandr_util.hpp"
#endif

#include "amd_perf_monitor.hpp"
#include "get_file_size.hpp"
#include "testbed.hpp"

namespace testbed
{

const char arg_prefix[]	= "-";
const char arg_app[]	= "app";


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

} // namespace testbed

static const char arg_nframes[]			= "frames";
static const char arg_screen[]			= "screen";
static const char arg_bitness[]			= "bitness";
static const char arg_config_id[]		= "config_id";
static const char arg_fsaa[]			= "fsaa";
static const char arg_skip[]			= "skip_frames";
static const char arg_grab[]			= "grab_frame";
static const char arg_drawcalls[]		= "drawcalls";
static const char arg_print_configs[]	= "print_egl_configs";
static const char arg_print_perf[]		= "print_perf_counters";

static Display* display;
static Atom wm_protocols;
static Atom wm_delete_window;


static uint64_t
timer_nsec()
{
#if defined(CLOCK_MONOTONIC_RAW)
	const clockid_t clockid = CLOCK_MONOTONIC_RAW;
#else
	const clockid_t clockid = CLOCK_MONOTONIC;
#endif

	timespec t;
	clock_gettime(clockid, &t);

	return t.tv_sec * 1000000000ULL + t.tv_nsec;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EGL helper
////////////////////////////////////////////////////////////////////////////////////////////////////

struct EGL
{
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;

	EGL()
	: display(EGL_NO_DISPLAY)
	, context(EGL_NO_CONTEXT)
	, surface(EGL_NO_SURFACE)
	{}

	bool initGLES2(
		Display* xdisplay,
		const Window& window,
		const unsigned config_id,
		const unsigned fsaa,
		const unsigned nbits_r,
		const unsigned nbits_g,
		const unsigned nbits_b,
		const unsigned nbits_a,
		const bool print_configs);

	void deinit();
	void swapBuffers() const;
};


static std::string
string_from_EGL_error(
	const EGLint error)
{
	switch (error) {

	case EGL_SUCCESS:
		return "EGL_SUCCESS";

	case EGL_NOT_INITIALIZED:
		return "EGL_NOT_INITIALIZED";

	case EGL_BAD_ACCESS:
		return "EGL_BAD_ACCESS";

	case EGL_BAD_ALLOC:
		return "EGL_BAD_ALLOC";

	case EGL_BAD_ATTRIBUTE:
		return "EGL_BAD_ATTRIBUTE";

	case EGL_BAD_CONTEXT:
		return "EGL_BAD_CONTEXT";

	case EGL_BAD_CONFIG:
		return "EGL_BAD_CONFIG";

	case EGL_BAD_CURRENT_SURFACE:
		return "EGL_BAD_CURRENT_SURFACE";

	case EGL_BAD_DISPLAY:
		return "EGL_BAD_DISPLAY";

	case EGL_BAD_SURFACE:
		return "EGL_BAD_SURFACE";

	case EGL_BAD_MATCH:
		return "EGL_BAD_MATCH";

	case EGL_BAD_PARAMETER:
		return "EGL_BAD_PARAMETER";

	case EGL_BAD_NATIVE_PIXMAP:
		return "EGL_BAD_NATIVE_PIXMAP";

	case EGL_BAD_NATIVE_WINDOW:
		return "EGL_BAD_NATIVE_WINDOW";

	case EGL_CONTEXT_LOST:
		return "EGL_CONTEXT_LOST";
	}

	std::ostringstream s;
	s << "unknown EGL error (0x" << std::hex << std::setw(8) << std::setfill('0') << error << ")";

	return s.str();
}


bool
testbed::util::reportEGLError(
	std::ostream& stream)
{
	const EGLint error = eglGetError();

	if (EGL_SUCCESS == error)
		return false;

	stream << "EGL error: " << 
		string_from_EGL_error(error) << std::endl;

	return true;
}

static std::string
string_from_EGL_attrib(
	const EGLint attr)
{
	switch (attr) {

	case EGL_BUFFER_SIZE:
		return "EGL_BUFFER_SIZE";

	case EGL_ALPHA_SIZE:
		return "EGL_ALPHA_SIZE";

	case EGL_BLUE_SIZE:
		return "EGL_BLUE_SIZE";

	case EGL_GREEN_SIZE:
		return "EGL_GREEN_SIZE";

	case EGL_RED_SIZE:
		return "EGL_RED_SIZE";

	case EGL_DEPTH_SIZE:
		return "EGL_DEPTH_SIZE";

	case EGL_STENCIL_SIZE:
		return "EGL_STENCIL_SIZE";

	case EGL_CONFIG_CAVEAT:
		return "EGL_CONFIG_CAVEAT";

	case EGL_CONFIG_ID:
		return "EGL_CONFIG_ID";

	case EGL_LEVEL:
		return "EGL_LEVEL";

	case EGL_MAX_PBUFFER_HEIGHT:
		return "EGL_MAX_PBUFFER_HEIGHT";

	case EGL_MAX_PBUFFER_PIXELS:
		return "EGL_MAX_PBUFFER_PIXELS";

	case EGL_MAX_PBUFFER_WIDTH:
		return "EGL_MAX_PBUFFER_WIDTH";

	case EGL_NATIVE_RENDERABLE:
		return "EGL_NATIVE_RENDERABLE";

	case EGL_NATIVE_VISUAL_ID:
		return "EGL_NATIVE_VISUAL_ID";

	case EGL_NATIVE_VISUAL_TYPE:
		return "EGL_NATIVE_VISUAL_TYPE";

	case EGL_SAMPLES:
		return "EGL_SAMPLES";

	case EGL_SAMPLE_BUFFERS:
		return "EGL_SAMPLE_BUFFERS";

	case EGL_SURFACE_TYPE:
		return "EGL_SURFACE_TYPE";

	case EGL_TRANSPARENT_TYPE:
		return "EGL_TRANSPARENT_TYPE";

	case EGL_TRANSPARENT_BLUE_VALUE:
		return "EGL_TRANSPARENT_BLUE_VALUE";

	case EGL_TRANSPARENT_GREEN_VALUE:
		return "EGL_TRANSPARENT_GREEN_VALUE";

	case EGL_TRANSPARENT_RED_VALUE:
		return "EGL_TRANSPARENT_RED_VALUE";

	case EGL_BIND_TO_TEXTURE_RGB:
		return "EGL_BIND_TO_TEXTURE_RGB";

	case EGL_BIND_TO_TEXTURE_RGBA:
		return "EGL_BIND_TO_TEXTURE_RGBA";

	case EGL_MIN_SWAP_INTERVAL:
		return "EGL_MIN_SWAP_INTERVAL";

	case EGL_MAX_SWAP_INTERVAL:
		return "EGL_MAX_SWAP_INTERVAL";

	case EGL_LUMINANCE_SIZE:
		return "EGL_LUMINANCE_SIZE";

	case EGL_ALPHA_MASK_SIZE:
		return "EGL_ALPHA_MASK_SIZE";

	case EGL_COLOR_BUFFER_TYPE:
		return "EGL_COLOR_BUFFER_TYPE";

	case EGL_RENDERABLE_TYPE:
		return "EGL_RENDERABLE_TYPE";

	case EGL_CONFORMANT:
		return "EGL_CONFORMANT";
	}

	std::ostringstream s;
	s << "unknown EGL attribute (0x" << std::hex << std::setw(8) << std::setfill('0') << attr << ")";

	return s.str();
}


uintptr_t
testbed::util::getNativeWindowSystem()
{
	return uintptr_t(display);
}

namespace testbed
{

template <>
class scoped_functor< EGL >
{
public:

	void operator()(EGL* arg)
	{
		util::reportEGLError();
		arg->deinit();
	}
};

} // namespace testbed

bool EGL::initGLES2(
	Display* xdisplay,
	const Window& window,
	const unsigned config_id,
	const unsigned fsaa,
	const unsigned nbits_r,
	const unsigned nbits_g,
	const unsigned nbits_b,
	const unsigned nbits_a,
	const bool print_configs)
{
	const unsigned nbits_pixel =
		nbits_r +
		nbits_g +
		nbits_b +
		nbits_a + 7 & ~7;

	if (0 == config_id && 0 == nbits_pixel)
	{
		std::cerr << "nil pixel size requested; abort" << std::endl;
		return false;
	}

	deinit();

	display = eglGetDisplay(xdisplay /*EGL_DEFAULT_DISPLAY*/);

	testbed::scoped_ptr< EGL, testbed::scoped_functor > deinit_self(this);

 	EGLint major;
 	EGLint minor;

	if (EGL_FALSE == eglInitialize(display, &major, &minor))
	{
		std::cerr << "eglInitialize() failed" << std::endl;
		return false;
	}

	std::cout << "eglInitialize() succeeded; major: " << major << ", minor: " << minor << std::endl;

	const char* str_version	= eglQueryString(display, EGL_VERSION);
	const char* str_vendor	= eglQueryString(display, EGL_VENDOR);
	const char* str_exten	= eglQueryString(display, EGL_EXTENSIONS);

	std::cout << "egl version, vendor, extensions:"
		"\n\t" << str_version <<
		"\n\t" << str_vendor <<
		"\n\t" << str_exten << std::endl;

	eglBindAPI(EGL_OPENGL_ES_API);

	EGLConfig config[128];
	EGLint num_config;

	if (EGL_FALSE == eglGetConfigs(display, config, EGLint(sizeof(config) / sizeof(config[0])), &num_config))
	{
		std::cerr << "eglGetConfigs() failed" << std::endl;
		return false;
	}

	for (EGLint i = 0; i < num_config && print_configs; ++i)
	{
		std::cout << "\nconfig " << i << ":\n";

		static const EGLint attr[] =
		{
			EGL_BUFFER_SIZE,
			EGL_ALPHA_SIZE,
			EGL_BLUE_SIZE,
			EGL_GREEN_SIZE,
			EGL_RED_SIZE,
			EGL_DEPTH_SIZE,
			EGL_STENCIL_SIZE,
			EGL_CONFIG_CAVEAT,
			EGL_CONFIG_ID,
			EGL_LEVEL,
			EGL_MAX_PBUFFER_HEIGHT,
			EGL_MAX_PBUFFER_PIXELS,
			EGL_MAX_PBUFFER_WIDTH,
			EGL_NATIVE_RENDERABLE,
			EGL_NATIVE_VISUAL_ID,
			EGL_NATIVE_VISUAL_TYPE,
			EGL_SAMPLES,
			EGL_SAMPLE_BUFFERS,
			EGL_SURFACE_TYPE,
			EGL_TRANSPARENT_TYPE,
			EGL_TRANSPARENT_BLUE_VALUE,
			EGL_TRANSPARENT_GREEN_VALUE,
			EGL_TRANSPARENT_RED_VALUE,
			EGL_BIND_TO_TEXTURE_RGB,
			EGL_BIND_TO_TEXTURE_RGBA,
			EGL_MIN_SWAP_INTERVAL,
			EGL_MAX_SWAP_INTERVAL,
			EGL_LUMINANCE_SIZE,
			EGL_ALPHA_MASK_SIZE,
			EGL_COLOR_BUFFER_TYPE,
			EGL_RENDERABLE_TYPE,
			EGL_CONFORMANT
		};

		for (unsigned j = 0; j < sizeof(attr) / sizeof(attr[0]); ++j)
		{
			EGLint value;

			if (EGL_FALSE == eglGetConfigAttrib(display, config[i], attr[j], &value))
			{
				std::cerr << "eglGetConfigAttrib() failed" << std::endl;
				continue;
			}

			std::cout << string_from_EGL_attrib(attr[j]) << " 0x" <<
				std::hex << std::setw(8) << std::setfill('0') << value << std::dec << std::endl;
		}
	}

	EGLint attr[64];
	unsigned na = 0;

	if (0 == config_id)
	{
		attr[na++] = EGL_BUFFER_SIZE;
		attr[na++] = EGLint(nbits_pixel);
		attr[na++] = EGL_RED_SIZE;
		attr[na++] = EGLint(nbits_r);
		attr[na++] = EGL_GREEN_SIZE;
		attr[na++] = EGLint(nbits_g);
		attr[na++] = EGL_BLUE_SIZE;
		attr[na++] = EGLint(nbits_b);
		attr[na++] = EGL_ALPHA_SIZE;
		attr[na++] = EGLint(nbits_a);
		attr[na++] = EGL_SURFACE_TYPE;
		attr[na++] = EGL_WINDOW_BIT;
		attr[na++] = EGL_RENDERABLE_TYPE;
		attr[na++] = EGL_OPENGL_ES2_BIT;

		if (testbed::hook::requires_depth())
		{
			attr[na++] = EGL_DEPTH_SIZE;
			attr[na++] = 16;
		}

		if (fsaa)
		{
			attr[na++] = EGL_SAMPLE_BUFFERS;
			attr[na++] = 1;
			attr[na++] = EGL_SAMPLES;
			attr[na++] = EGLint(fsaa);
		}
		else
		{
			attr[na++] = EGL_CONFIG_CAVEAT;
			attr[na++] = EGL_NONE;
		}
	}
	else
	{
		attr[na++] = EGL_CONFIG_ID;
		attr[na++] = EGLint(config_id);
	}

	assert(na < sizeof(attr) / sizeof(attr[0]));

	attr[na] = EGL_NONE;

	if (EGL_FALSE == eglChooseConfig(display, attr, config,
			EGLint(sizeof(config) / sizeof(config[0])), &num_config))
	{
		std::cerr << "eglChooseConfig() failed" << std::endl;
		return false;
	}

	unsigned best_match = 0;

	for (EGLint i = 1; i < num_config; ++i)
	{
		EGLint value[2];

		eglGetConfigAttrib(display, config[best_match], EGL_CONFIG_CAVEAT, &value[0]);
		eglGetConfigAttrib(display, config[i], EGL_CONFIG_CAVEAT, &value[1]);

		if (value[1] == EGL_SLOW_CONFIG)
			continue;

		if (value[0] == EGL_SLOW_CONFIG)
		{
			best_match = i;
			continue;
		}

		eglGetConfigAttrib(display, config[best_match], EGL_BUFFER_SIZE, &value[0]);
		eglGetConfigAttrib(display, config[i], EGL_BUFFER_SIZE, &value[1]);

		if (value[0] < value[1])
			continue;

		eglGetConfigAttrib(display, config[best_match], EGL_ALPHA_SIZE, &value[0]);
		eglGetConfigAttrib(display, config[i], EGL_ALPHA_SIZE, &value[1]);

		if (value[0] < value[1])
			continue;

		eglGetConfigAttrib(display, config[best_match], EGL_DEPTH_SIZE, &value[0]);
		eglGetConfigAttrib(display, config[i], EGL_DEPTH_SIZE, &value[1]);

		if (value[0] < value[1])
			continue;

		eglGetConfigAttrib(display, config[best_match], EGL_SAMPLES, &value[0]);
		eglGetConfigAttrib(display, config[i], EGL_SAMPLES, &value[1]);

		if (value[0] < value[1])
			continue;

		best_match = i;
	}

	EGLint value;
	eglGetConfigAttrib(display, config[best_match], EGL_CONFIG_ID, &value);

	std::cout << "\nchoosing configs returned " << num_config << " candidate(s), best match is EGL_CONFIG_ID 0x" <<
		std::hex << std::setw(8) << std::setfill('0') << value << std::dec << "\n" << std::endl;

	const EGLint context_attr[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	context = eglCreateContext(display, config[best_match], EGL_NO_CONTEXT, context_attr);

	if (EGL_NO_CONTEXT == context)
	{
		std::cerr << "eglCreateContext() failed" << std::endl;
		return false;
	}

	surface = eglCreateWindowSurface(display, config[best_match], window, 0);

	if (EGL_NO_SURFACE == surface)
	{
		std::cerr << "eglCreateWindowSurface() failed" << std::endl;
		return false;
	}

	if (EGL_FALSE == eglSurfaceAttrib(display, surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED))
	{
		std::cerr << "eglSurfaceAttrib() failed" << std::endl;
		return false;
	}

	if (EGL_FALSE == eglMakeCurrent(display, surface, surface, context))
	{
		std::cerr << "eglMakeCurrent() failed" << std::endl;
		return false;
	}

	deinit_self.reset();
	return true;
}


void EGL::deinit()
{
	if (EGL_NO_SURFACE != surface)
	{
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroySurface(display, surface);

		surface = EGL_NO_SURFACE;
	}

	if (EGL_NO_CONTEXT != context)
	{
		eglDestroyContext(display, context);

		context = EGL_NO_CONTEXT;
	}

	if (EGL_NO_DISPLAY != display)
	{
		eglTerminate(display);

		display = EGL_NO_DISPLAY;
	}
}


void EGL::swapBuffers() const
{
	eglSwapBuffers(display, surface);
}


static std::string
string_from_GL_error(
	const GLenum error)
{
	switch (error) {

	case GL_NO_ERROR:
		return "GL_NO_ERROR";

	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";

	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return "GL_INVALID_FRAMEBUFFER_OPERATION";

	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";

	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";

	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";
	}

	std::ostringstream s;
	s << "unknown GL error (0x" << std::hex << std::setw(8) << std::setfill('0') << error << ")";

	return s.str();
}


bool
testbed::util::reportGLError(
	std::ostream& stream)
{
	const GLenum error = glGetError();

	if (GL_NO_ERROR == error)
		return false;

	stream << "GL error: " << string_from_GL_error(error) << std::endl;

	return true;
}


static bool
reportGLCaps()
{
	const GLubyte* str_version	= glGetString(GL_VERSION);
	const GLubyte* str_vendor	= glGetString(GL_VENDOR);
	const GLubyte* str_renderer	= glGetString(GL_RENDERER);
	const GLubyte* str_glsl_ver	= glGetString(GL_SHADING_LANGUAGE_VERSION);
	const GLubyte* str_exten	= glGetString(GL_EXTENSIONS);

	std::cout << "gl version, vendor, renderer, glsl version, extensions:"
		"\n\t" << (const char*) str_version <<
		"\n\t" << (const char*) str_vendor << 
		"\n\t" << (const char*) str_renderer <<
		"\n\t" << (const char*) str_glsl_ver <<
		"\n\t" << (const char*) str_exten << "\n" << std::endl;

	GLint params[2]; // we won't need more than 2

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, params);
	std::cout << "GL_MAX_TEXTURE_SIZE: " << params[0] << std::endl;

	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, params);
	std::cout << "GL_MAX_CUBE_MAP_TEXTURE_SIZE: " << params[0] << std::endl;

	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, params);
	std::cout << "GL_MAX_VIEWPORT_DIMS: " << params[0] << ", " << params[1] << std::endl;

	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, params);
	std::cout << "GL_MAX_RENDERBUFFER_SIZE: " << params[0] << std::endl;

	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, params);
	std::cout << "GL_MAX_VERTEX_ATTRIBS: " << params[0] << std::endl;

	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, params);
	std::cout << "GL_MAX_VERTEX_UNIFORM_VECTORS: " << params[0] << std::endl;

	glGetIntegerv(GL_MAX_VARYING_VECTORS, params);
	std::cout << "GL_MAX_VARYING_VECTORS: " << params[0] << std::endl;

	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, params);
	std::cout << "GL_MAX_FRAGMENT_UNIFORM_VECTORS: " << params[0] << std::endl;

	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, params);
	std::cout << "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: " << params[0] << std::endl;

	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, params);
	std::cout << "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS: " << params[0] << std::endl;

	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, params);
	std::cout << "GL_MAX_TEXTURE_IMAGE_UNITS: " << params[0] << std::endl;

	std::cout << std::endl;

	return true;
}


static bool
setupShaderFromString(
	const GLuint shader_name,
	const char* const source,
	const size_t length)
{
	assert(0 != source);
	assert(0 != length);

	if (GL_FALSE == glIsShader(shader_name))
	{
		std::cerr << __FUNCTION__ <<
			" argument is not a valid shader object" << std::endl;
		return false;
	}

	GLboolean compiler_present = GL_FALSE;
	glGetBooleanv(GL_SHADER_COMPILER, &compiler_present);

	if (GL_TRUE != compiler_present)
	{
		std::cerr << "no shader compiler present (binary only)" << std::endl;
		return false;
	}

	const char* src = source;
	const GLint src_len = (GLint) length;

	glShaderSource(shader_name, 1, &src, &src_len);
	glCompileShader(shader_name);

	if (testbed::util::reportGLError())
	{
		std::cerr << "cannot compile shader from source (binary only?)" << std::endl;
		return false;
	}

	const GLsizei log_max_len = 2048;
	static char log[log_max_len + 1];
	GLsizei log_len;

	glGetShaderInfoLog(shader_name, log_max_len, &log_len, log);

	log[log_len] = '\0';
	std::cerr << "shader compile log: " << log << std::endl;

	GLint success = GL_FALSE;
	glGetShaderiv(shader_name, GL_COMPILE_STATUS, &success);

	return GL_TRUE == success;
}


bool
testbed::util::setupShader(
	const GLuint shader_name,
	const char* const filename)
{
	assert(0 != filename);

	size_t length;
	const scoped_ptr< char, generic_free > source(
		get_buffer_from_file(filename, length));

	if (0 == source())
	{
		std::cerr << __FUNCTION__ <<
			" failed to read shader file '" << filename << "'" << std::endl;
		return false;
	}

	return setupShaderFromString(shader_name, source(), length);
}


bool
testbed::util::setupShaderWithPatch(
	const GLuint shader_name,
	const char* const filename,
	const size_t patch_count,
	const std::string* const patch)
{
	assert(0 != filename);
	assert(0 != patch);

	size_t length;
	const scoped_ptr< char, generic_free > source(
		get_buffer_from_file(filename, length));

	if (0 == source())
	{
		std::cerr << __FUNCTION__ <<
			" failed to read shader file '" << filename << "'" << std::endl;
		return false;
	}

	std::string src_final(source(), length);
	size_t npatched = 0;

	for (size_t i = 0; i < patch_count; ++i)
	{
		const std::string& patch_src = patch[i * 2 + 0];
		const std::string& patch_dst = patch[i * 2 + 1];

		if (patch_src.empty() || patch_src == patch_dst)
			continue;

		const size_t len_src = patch_src.length();
		const size_t len_dst = patch_dst.length();

		std::cout << "turn: " << patch_src << "\ninto: " << patch_dst << std::endl;

		for (size_t pos = src_final.find(patch_src);
			std::string::npos != pos;
			pos = src_final.find(patch_src, pos + len_dst))
		{
			src_final.replace(pos, len_src, patch_dst);
			++npatched;
		}
	}

	std::cout << "substitutions: " << npatched << std::endl;

	return setupShaderFromString(shader_name, src_final.c_str(), src_final.length());
}


bool
testbed::util::setupProgram(
	const GLuint prog,
	const GLuint shader_vert,
	const GLuint shader_frag)
{
	if (GL_FALSE == glIsProgram(prog))
	{
		std::cerr << __FUNCTION__ <<
			" argument is not a valid program object" << std::endl;
		return false;
	}

	if (GL_FALSE == glIsShader(shader_vert) ||
		GL_FALSE == glIsShader(shader_frag))
	{
		std::cerr << __FUNCTION__ <<
			" argument is not a valid shader object" << std::endl;
		return false;
	}

	glAttachShader(prog, shader_vert);
	glAttachShader(prog, shader_frag);

	glLinkProgram(prog);

	const GLsizei log_max_len = 1024;
	static char log[log_max_len + 1];
	GLsizei log_len;

	glGetProgramInfoLog(prog, log_max_len, &log_len, log);

	log[log_len] = '\0';
	std::cerr << "shader link log: " << log << std::endl;

	GLint success = GL_FALSE;
	glGetProgramiv(prog, GL_LINK_STATUS, &success);

	return GL_TRUE == success;
}


static bool
saveFramebuffer(
	const unsigned window_w,
	const unsigned window_h,
	const char* name = NULL)
{
	const unsigned pixels_len = 4 * window_h * window_w;
	testbed::scoped_ptr< GLvoid, testbed::generic_free > pixels(malloc(pixels_len));

	glReadPixels(0, 0, window_w, window_h, GL_RGBA, GL_UNSIGNED_BYTE, pixels());

	const time_t t = time(NULL);
	const tm* tt = localtime(&t);

	char default_name[FILENAME_MAX + 1];

	if (!name || !name[0])
	{
		const char nameBase[] = "frame";
		const char nameExt[] = ".raw";

		sprintf(default_name, "%s%04d%03d%02d%02d%02d%s",
			nameBase, tt->tm_year, tt->tm_yday, tt->tm_hour, tt->tm_min, tt->tm_sec, nameExt);

		name = default_name;
	}

	std::cout << "saving framegrab as '" << name << "'" << std::endl;

	testbed::scoped_ptr< FILE, testbed::scoped_functor > file(fopen(name, "wb"));

	if (0 == file())
	{
		std::cerr << "failure opening framegrab file '" << name << "'" << std::endl;
		return false;
	}

	const unsigned dims[] = { window_h, window_w };

	if (1 != fwrite(dims, sizeof(dims), 1, file()) ||
		1 != fwrite(pixels(), pixels_len, 1, file()))
	{
		std::cerr << "failure writing framegrab file '" << name << "'" << std::endl;
		return false;
	}

	return true;
}


static bool
validate_fullscreen(
	const char* string,
	unsigned &screen_w,
	unsigned &screen_h)
{
	if (!string)
		return false;

	unsigned x, y, hz;

	if (3 != sscanf(string, "%u %u %u", &x, &y, &hz))
		return false;

	if (!x || !y || !hz)
		return false;

	screen_w = x;
	screen_h = y;

	return true;
}


static bool
validate_bitness(
	const char* string,
	unsigned (&screen_bitness)[4])
{
	if (!string)
		return false;

	unsigned bitness[4];

	if (4 != sscanf(string, "%u %u %u %u", &bitness[0], &bitness[1], &bitness[2], &bitness[3]))
		return false;

	if (!bitness[0] || 16 < bitness[0] ||
		!bitness[1] || 16 < bitness[1] ||
		!bitness[2] || 16 < bitness[2] ||
		16 < bitness[3])
	{
		return false;
	}

	screen_bitness[0] = bitness[0];
	screen_bitness[1] = bitness[1];
	screen_bitness[2] = bitness[2];
	screen_bitness[3] = bitness[3];

	return true;
}


static bool
processEvents(
	Display* display,
	const Window& window)
{
	XEvent event;

	if (!XCheckTypedWindowEvent(display, window, ClientMessage, &event))
		return true;

	if (event.xclient.message_type == wm_protocols &&
		event.xclient.data.l[0] == wm_delete_window)
	{
		return false;
	}

	XPutBackEvent(display, &event);

	return true;
}


static inline unsigned
bitcount(
	unsigned n)
{
	unsigned count = 0;

	while (n)
	{
		count++;
		n &= (n - 1);
	}

	return count;
}


int main(
	int argc,
	char** argv)
{
	unsigned fsaa = 0;
	unsigned frames = -1U;
	unsigned skip_frames = 0;
	unsigned grab_frame = -1U;
	char grab_filename[FILENAME_MAX + 1] = { 0 };
	unsigned w = 512, h = 512;
	unsigned bitness[4] = { 0 };
	unsigned config_id = 0;
	unsigned drawcalls = 0;

	bool cli_err = false;
	bool print_configs = false;
	bool print_perf_counters = false;

	const unsigned prefix_len = strlen(testbed::arg_prefix);

	for (int i = 1; i < argc && !cli_err; ++i)
	{
		if (strncmp(argv[i], testbed::arg_prefix, prefix_len))
		{
			cli_err = true;
			continue;
		}

		if (!strcmp(argv[i] + prefix_len, testbed::arg_app))
		{
			if (++i == argc)
				cli_err = true;

			continue;
		}

		if (!strcmp(argv[i] + prefix_len, arg_nframes))
		{
			if (!(++i < argc) || (1 != sscanf(argv[i], "%u", &frames)))
				cli_err = true;

			continue;
		}

		if (!strcmp(argv[i] + prefix_len, arg_screen))
		{
			if (!(++i < argc) || !validate_fullscreen(argv[i], w, h))
				cli_err = true;

			continue;
		}

		if (!strcmp(argv[i] + prefix_len, arg_bitness))
		{
			if (!(++i < argc) || !validate_bitness(argv[i], bitness))
				cli_err = true;

			continue;
		}

		if (!strcmp(argv[i] + prefix_len, arg_config_id))
		{
			if (!(++i < argc) || (1 != sscanf(argv[i], "%u", &config_id)) || 0 == config_id)
				cli_err = true;

			continue;
		}

		if (!strcmp(argv[i] + prefix_len, arg_fsaa))
		{
			if (!(++i < argc) || (1 != sscanf(argv[i], "%u", &fsaa)))
				cli_err = true;

			continue;
		}

		if (!strcmp(argv[i] + prefix_len, arg_skip))
		{
			if (!(++i < argc) || (1 != sscanf(argv[i], "%u", &skip_frames)))
				cli_err = true;

			continue;
		}

		if (!strcmp(argv[i] + prefix_len, arg_grab))
		{
			if (!(++i < argc) || !sscanf(argv[i], "%u %" XQUOTE(FILENAME_MAX) "s", &grab_frame, grab_filename))
				cli_err = true;

			continue;
		}

		if (!strcmp(argv[i] + prefix_len, arg_drawcalls))
		{
			if (!(++i < argc) || (1 != sscanf(argv[i], "%u", &drawcalls)) || !drawcalls)
				cli_err = true;

			continue;
		}

		if (!strcmp(argv[i] + prefix_len, arg_print_configs))
		{
			print_configs = true;
			continue;
		}

		if (!strcmp(argv[i] + prefix_len, arg_print_perf))
		{
			print_perf_counters = true;
			continue;
		}

		cli_err = true;
	}

	if (cli_err)
	{
		std::cerr << "usage: " << argv[0] << " [<option> ...]\n"
			"options (multiple args to an option must constitute a single string, eg. -foo \"a b c\"):\n"
			"\t" << testbed::arg_prefix << arg_nframes <<
			" <unsigned_integer>\t\t: set number of frames to run; default is max unsigned int\n"
			"\t" << testbed::arg_prefix << arg_screen <<
			" <width> <height> <Hz>\t\t: set fullscreen output of specified geometry and refresh\n"
			"\t" << testbed::arg_prefix << arg_bitness <<
			" <r> <g> <b> <a>\t\t: set EGL config of specified RGBA bitness; default is screen's bitness\n"
			"\t" << testbed::arg_prefix << arg_config_id <<
			" <positive_integer>\t\t: set EGL config of specified ID; overrides any other config options\n"
			"\t" << testbed::arg_prefix << arg_fsaa <<
			" <positive_integer>\t\t: set fullscreen antialiasing; default is none\n"
			"\t" << testbed::arg_prefix << arg_skip <<
			" <unsigned_integer>\t\t: display every (2^N)th frame, draw the rest to completion via glFinish\n"
			"\t" << testbed::arg_prefix << arg_grab <<
			" <unsigned_integer> [<file>]\t: grab the Nth frame to file; index is zero-based\n"
			"\t" << testbed::arg_prefix << arg_drawcalls <<
			" <positive_integer>\t\t: set number of drawcalls per frame; may be ignored by apps\n"
			"\t" << testbed::arg_prefix << arg_print_configs <<
			"\t\t\t: print available EGL configs\n"
			"\t" << testbed::arg_prefix << arg_print_perf <<
			"\t\t\t: print available GPU performance monitor groups and counters\n"
			"\t" << testbed::arg_prefix << testbed::arg_app <<
			" <option> [<arguments>]\t\t: app-specific option" << std::endl;

		return 1;
	}

	skip_frames = (1 << skip_frames) - 1;

	if (drawcalls && !testbed::hook::set_num_drawcalls(drawcalls))
		std::cerr << "drawcalls argument ignored by app" << std::endl;

	display = XOpenDisplay(NULL);

	if (!display)
	{
		std::cerr << "failed to open display" << std::endl;
		return 1;
	}

	if (0 == config_id && 0 == bitness[0])
	{
		const int screen = XDefaultScreen(display);

		XVisualInfo vinfo_template;
		memset(&vinfo_template, 0, sizeof(vinfo_template));

		vinfo_template.visualid = XVisualIDFromVisual(XDefaultVisual(display, screen));
		vinfo_template.screen = screen;

		const long vinfo_mask = VisualIDMask | VisualScreenMask;

		int n_vinfo = 0;
		XVisualInfo* vinfo = XGetVisualInfo(display, vinfo_mask, &vinfo_template, &n_vinfo);

		bitness[0] = bitcount(vinfo->red_mask);
		bitness[1] = bitcount(vinfo->green_mask);
		bitness[2] = bitcount(vinfo->blue_mask);
		bitness[3] = 0;

		XFree(vinfo);

		std::cout << "Using screen RGB bitness: " <<
			bitness[0] << " " <<
			bitness[1] << " " <<
			bitness[2] << std::endl;
	}

	const Window root = XDefaultRootWindow(display);

#ifdef USE_XRANDR
	util::set_screen_config(display, root, w, h);
#endif

	const unsigned border_width = 0;

	const int x = 0, y = 0;
	const unsigned long border = 0, background = 0;

	const Window window = XCreateSimpleWindow(display, root, x, y, w, h, border_width, border, background);

	if (!window)
	{
		XCloseDisplay(display);

		std::cerr << "failed to create a window" << std::endl;
		return 1;
	}

	wm_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
	wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);

	XSetWMProtocols(display, window, &wm_delete_window, 1);

	XMapWindow(display, window);
	XFlush(display);

	int exit_code = 0;
	EGL egl;

	if (egl.initGLES2(
			display,
			window,
			config_id,
			fsaa,
			bitness[0],
			bitness[1],
			bitness[2],
			bitness[3],
			print_configs) &&
		reportGLCaps() &&
		testbed::hook::init_resources(argc, argv))
	{
		amd_perf_monitor::load_extension();
		amd_perf_monitor::peek_performance_monitor(print_perf_counters);

		unsigned nframes = 0;
		const uint64_t t0 = timer_nsec();

		while (processEvents(display, window) &&
			nframes < frames)
		{
			if (!testbed::hook::render_frame())
			{
				std::cerr << "failed rendering a frame; bailing out" << std::endl;
				exit_code = 2;
				break;
			}

			if (nframes == grab_frame)
				saveFramebuffer(w, h, grab_filename);

			if (nframes & skip_frames)
				glFlush();
			else
				egl.swapBuffers();

			++nframes;
		}

		glFinish();

		const uint64_t dt = timer_nsec() - t0;

		std::cout << "total frames rendered: " << nframes <<
			"\ndrawcalls per frame: " << testbed::hook::get_num_drawcalls() << std::endl;

		if (dt)
		{
			const double sec = double(dt) * 1e-9;

			std::cout << "elapsed time: " << sec << " s"
				"\naverage FPS: " << (double(nframes) / sec) << std::endl;
		}

		amd_perf_monitor::depeek_performance_monitor();

		testbed::hook::deinit_resources();
	}
	else
	{
		std::cerr << "failed to initialise either GLES or resources; bailing out" << std::endl;
		exit_code = 1;
	}

	egl.deinit();

	XDestroyWindow(display, window);
	XCloseDisplay(display);

	return exit_code;
}
