#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#if GLX_ARB_create_context == 0
#error Missing required extension GLX_ARB_create_context.
#endif 

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
#include "scoped.hpp"
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

static const char arg_nframes[]		= "frames";
static const char arg_screen[]		= "screen";
static const char arg_bitness[]		= "bitness";
static const char arg_fsaa[]		= "fsaa";
static const char arg_skip[]		= "skip_frames";
static const char arg_grab[]		= "grab_frame";
static const char arg_drawcalls[]	= "drawcalls";
static const char arg_print_perf[]	= "print_perf_counters";

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

	std::cout << "gl version, vendor, renderer, glsl version, extensions:"
		"\n\t" << (const char*) str_version <<
		"\n\t" << (const char*) str_vendor << 
		"\n\t" << (const char*) str_renderer <<
		"\n\t" << (const char*) str_glsl_ver <<
		"\n\t";

	GLint num_extensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);

	if (0 != num_extensions)
	{
		std::cout << (const char*) glGetStringi(GL_EXTENSIONS, 0);

		for (GLuint i = 1; i != (GLuint) num_extensions; ++i)
			std::cout << ' ' << (const char*) glGetStringi(GL_EXTENSIONS, i);

		std::cout << '\n' << std::endl;
	}
	else
		std::cout << "nil\n" << std::endl;

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

	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, params);
	std::cout << "GL_MAX_VERTEX_UNIFORM_COMPONENTS: " << params[0] << std::endl;

#if 0
	glGetIntegerv(GL_MAX_VARYING_COMPONENTS, params);
	std::cout << "GL_MAX_VARYING_COMPONENTS: " << params[0] << std::endl;
#endif

	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, params);
	std::cout << "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS: " << params[0] << std::endl;

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

	const char* src[] =
	{
		"#version 150\n",
		source
	};

	const GLint src_len[] =
	{
		(GLint) strlen(src[0]),
		(GLint) length
	};

	glShaderSource(shader_name, sizeof(src) / sizeof(src[0]), src, src_len);
	glCompileShader(shader_name);

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
		event.xclient.data.l[0] == long(wm_delete_window))
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
	unsigned drawcalls = 0;

	bool cli_err = false;
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
			" <r> <g> <b> <a>\t\t: set GLX config of specified RGBA bitness; default is screen's bitness\n"
			"\t" << testbed::arg_prefix << arg_fsaa <<
			" <positive_integer>\t\t: set fullscreen antialiasing; default is none\n"
			"\t" << testbed::arg_prefix << arg_skip <<
			" <unsigned_integer>\t\t: display every (2^N)th frame, draw the rest to completion via glFinish\n"
			"\t" << testbed::arg_prefix << arg_grab <<
			" <unsigned_integer> [<file>]\t: grab the Nth frame to file; index is zero-based\n"
			"\t" << testbed::arg_prefix << arg_drawcalls <<
			" <positive_integer>\t\t: set number of drawcalls per frame; may be ignored by apps\n"
			"\t" << testbed::arg_prefix << arg_print_perf <<
			"\t\t\t: print available GPU performance monitor groups and counters\n"
			"\t" << testbed::arg_prefix << testbed::arg_app <<
			" <option> [<arguments>]\t\t: app-specific option" << std::endl;

		return 1;
	}

	skip_frames = (1 << skip_frames) - 1;

	if (drawcalls && !testbed::hook::set_num_drawcalls(drawcalls))
		std::cerr << "drawcalls argument ignored by app" << std::endl;

	Display* display = XOpenDisplay(NULL);

	if (NULL == display)
	{
		std::cerr << "failed to open display" << std::endl;
		return 1;
	}

	if (!bitness[0])
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

	const char* extensions = glXQueryExtensionsString(display, DefaultScreen(display));
	std::cout << extensions << std::endl;

	int visual_attr[64];
	unsigned na = 0;

	visual_attr[na++] = GLX_X_RENDERABLE;
	visual_attr[na++] = True;
	visual_attr[na++] = GLX_DRAWABLE_TYPE;
	visual_attr[na++] = GLX_WINDOW_BIT;
	visual_attr[na++] = GLX_RENDER_TYPE;
	visual_attr[na++] = GLX_RGBA_BIT;
	visual_attr[na++] = GLX_X_VISUAL_TYPE;
	visual_attr[na++] = GLX_TRUE_COLOR;
	visual_attr[na++] = GLX_RED_SIZE;
	visual_attr[na++] = bitness[0];
	visual_attr[na++] = GLX_GREEN_SIZE;
	visual_attr[na++] = bitness[1];
	visual_attr[na++] = GLX_BLUE_SIZE;
	visual_attr[na++] = bitness[2];
	visual_attr[na++] = GLX_ALPHA_SIZE;
	visual_attr[na++] = bitness[3];
	visual_attr[na++] = GLX_DOUBLEBUFFER;
	visual_attr[na++] = True;

	if (fsaa)
	{
		visual_attr[na++] = GLX_SAMPLE_BUFFERS;
		visual_attr[na++] = 1;
		visual_attr[na++] = GLX_SAMPLES;
		visual_attr[na++] = fsaa;
	}

	assert(na < sizeof(visual_attr) / sizeof(visual_attr[0]));

	visual_attr[na] = None;

	int fbcount;
	const GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attr, &fbcount);

	if (NULL == fbc)
	{
		std::cerr << "failed to retrieve a framebuffer config" << std::endl;
		return 1;
	}

	const XVisualInfo* vi = glXGetVisualFromFBConfig(display, fbc[0]);

	XSetWindowAttributes swa;
	swa.colormap = XCreateColormap(display, root, vi->visual, AllocNone);
	swa.border_pixel = 0;
	swa.event_mask = StructureNotifyMask;

	const unsigned border_width = 0;

	const int x = 0, y = 0;

	const Window window = XCreateWindow(display, root, x, y, w, h, border_width, vi->depth,
		InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);

	if (0 == window)
	{
		std::cerr << "failed to create a window" << std::endl;
		return 1;
	}

	wm_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
	wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);

	XSetWMProtocols(display, window, &wm_delete_window, 1);

	XMapWindow(display, window);
	XFlush(display);

	static const int context_attribs[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 2,
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, 
		None
	};

	const GLXContext context = glXCreateContextAttribsARB(display, fbc[0], NULL, True, context_attribs);

	if (NULL == context)
	{
		std::cerr << "failed to create GL context" << std::endl;
		return 1;
	}

	glXMakeCurrent(display, window, context);

	if (testbed::util::reportGLError())
	{
		std::cerr << "warning: gl context started with errors; wiping the slate clean" << std::endl;
		while (GL_NO_ERROR != glGetError());
	}

	int exit_code = 0;

	if (reportGLCaps() &&
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
				glXSwapBuffers(display, window);

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

	glXMakeCurrent(display, None, NULL);
	glXDestroyContext(display, context);

	XDestroyWindow(display, window);
	XCloseDisplay(display);

	return exit_code;
}
