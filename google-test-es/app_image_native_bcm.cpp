#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglext_brcm.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "pure_macro.hpp"

#if EGL_KHR_image == 0
#error Missing required extension EGL_KHR_image.
#endif

#if GL_OES_EGL_image == 0
#error Missing required extension GL_OES_EGL_image.
#endif

#if EGL_KHR_lock_surface == 0
#error Missing required extension EGL_KHR_lock_surface.
#endif

#if EGL_BRCM_global_image == 0
#error Missing required extension EGL_BRCM_global_image.
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <iostream>

#include "testbed.hpp"

#include "rendVertAttr.hpp"

namespace
{

#define SETUP_VERTEX_ATTR_POINTERS_MASK	(			\
		SETUP_VERTEX_ATTR_POINTERS_MASK_vertex |	\
		SETUP_VERTEX_ATTR_POINTERS_MASK_tcoord)

#include "rendVertAttr_setupVertAttrPointers.hpp"
#undef SETUP_VERTEX_ATTR_POINTERS_MASK

struct Vertex
{
	GLfloat pos[3];
	GLfloat txc[2];
};

} // namespace

namespace testbed
{

#define OPTION_IDENTIFIER_MAX	64

static const char arg_image_dim[] = "image_dim";

static unsigned g_image_w = 256;
static unsigned g_image_h = 256;

static EGLDisplay g_display = EGL_NO_DISPLAY;
static EGLContext g_context = EGL_NO_CONTEXT;

static EGLImageKHR g_image[] =
{
	EGL_NO_IMAGE_KHR,
	EGL_NO_IMAGE_KHR
};

static const unsigned num_images = sizeof(g_image) / sizeof(g_image[0]);
static EGLint g_pixmap[num_images][5];	// magic size revealed by gazing at the stars
static EGLSurface g_surface[num_images];

enum {
	PROG_QUAD,

	PROG_COUNT,
	PROG_FORCE_UINT = -1U
};

enum {
	UNI_SAMPLER_ALBEDO,

	UNI_COUNT,
	UNI_FORCE_UINT = -1U
};

enum {
	MESH_QUAD,

	MESH_COUNT,
	MESH_FORCE_UINT = -1U
};

enum {
	VBO_QUAD_VTX,
	VBO_QUAD_IDX,

	VBO_COUNT,
	VBO_FORCE_UINT = -1U
};

static GLint g_uni[PROG_COUNT][UNI_COUNT];

static GLuint g_tex[num_images];
static GLuint g_vbo[VBO_COUNT];
static GLuint g_shader_vert[PROG_COUNT];
static GLuint g_shader_frag[PROG_COUNT];
static GLuint g_shader_prog[PROG_COUNT];

static unsigned g_num_faces[MESH_COUNT];
static unsigned g_num_drawcalls = 1;

static rend::ActiveAttrSemantics g_active_attr_semantics[PROG_COUNT];


bool
hook::set_num_drawcalls(
	const unsigned n)
{
	g_num_drawcalls = n;

	return true;
}


unsigned
hook::get_num_drawcalls()
{
	return g_num_drawcalls;
}


bool
hook::requires_depth()
{
	return false;
}


static bool
parse_cli(
	const unsigned argc,
	const char* const * argv)
{
	bool cli_err = false;

	const unsigned prefix_len = strlen(arg_prefix);

	for (unsigned i = 1; i < argc && !cli_err; ++i)
	{
		if (strncmp(argv[i], arg_prefix, prefix_len) ||
			strcmp(argv[i] + prefix_len, arg_app))
		{
			continue;
		}

		if (++i < argc)
		{
			char option[OPTION_IDENTIFIER_MAX + 1];
			int opt_arg_start;

			if (1 == sscanf(argv[i], "%" XQUOTE(OPTION_IDENTIFIER_MAX) "s %n", option, &opt_arg_start))
			{
				if (!strcmp(option, arg_image_dim))
					if (2 == sscanf(argv[i] + opt_arg_start, "%u %u", &g_image_w, &g_image_h))
					{
						continue;
					}
			}
		}

		cli_err = true;
	}

	if (cli_err)
	{
		std::cerr << "app options (multiple args to an option must constitute a single string, eg. -foo \"a b c\"):\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_image_dim <<
			" <width> <height>\t: use specified dimensions with source image; width must be a multiple of 32\n" << std::endl;
	}

	return !cli_err;
}


static bool
createIndexedGridQuad(
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	unsigned& num_faces)
{
	assert(vbo_arr && vbo_idx);

	// thatch a grid of the following dimensions:
	const int rows = 2;
	const int cols = 2;

	assert(rows > 1);
	assert(cols > 1);

	static Vertex arr[rows * cols];
	unsigned ai = 0;

	g_num_faces[MESH_QUAD] = (rows - 1) * (cols - 1) * 2;

	for (int i = 0; i < rows; ++i)
		for (int j = 0; j < cols; ++j)
		{
			assert(ai < sizeof(arr) / sizeof(arr[0]));

			arr[ai].pos[0] = -1.f + j * (2.f / (cols - 1));
			arr[ai].pos[1] =  1.f - i * (2.f / (rows - 1));
			arr[ai].pos[2] = 0.f;
			arr[ai].txc[0] = (float) j / (cols - 1);
			arr[ai].txc[1] = (float) i / (rows - 1);

			++ai;
		}

	assert(ai == sizeof(arr) / sizeof(arr[0]));

	static uint16_t idx[(rows - 1) * (cols - 1) * 2][3];
	unsigned ii = 0;

	for (int i = 0; i < rows - 1; ++i)
		for (int j = 0; j < cols - 1; ++j)
		{
			idx[ii][0] = (uint16_t)(j + i * cols);
			idx[ii][1] = (uint16_t)(j + (i + 1) * cols);
			idx[ii][2] = (uint16_t)(j + i * cols + 1);

			++ii;

			idx[ii][0] = (uint16_t)(j + i * cols + 1);
			idx[ii][1] = (uint16_t)(j + (i + 1) * cols);
			idx[ii][2] = (uint16_t)(j + (i + 1) * cols + 1);

			++ii;
		}

	assert(ii == sizeof(idx) / sizeof(idx[0]));

	const unsigned num_verts = sizeof(arr) / sizeof(arr[0]);
	const unsigned num_indes = sizeof(idx) / sizeof(idx[0][0]);

	std::cout << "number of vertices: " << num_verts <<
		"\nnumber of indices: " << num_indes << std::endl;

	glBindBuffer(GL_ARRAY_BUFFER, vbo_arr);
	glBufferData(GL_ARRAY_BUFFER, sizeof(arr), arr, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (util::reportGLError())
	{
		std::cerr << __FUNCTION__ <<
			" failed at glBindBuffer/glBufferData for ARRAY_BUFFER" << std::endl;
		return false;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_idx);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	if (util::reportGLError())
	{
		std::cerr << __FUNCTION__ <<
			" failed at glBindBuffer/glBufferData for ELEMENT_ARRAY_BUFFER" << std::endl;
		return false;
	}

	return true;
}


static unsigned
findOptimalConfig(
		const EGLConfig* const config,
		const EGLint num_config)
{
	unsigned best_match = 0;

	for (EGLint i = 1; i < num_config; ++i)
	{
		EGLint value[2];

		eglGetConfigAttrib(g_display, config[best_match], EGL_CONFIG_CAVEAT, &value[0]);
		eglGetConfigAttrib(g_display, config[i], EGL_CONFIG_CAVEAT, &value[1]);

		if (value[1] == EGL_SLOW_CONFIG)
			continue;

		if (value[0] == EGL_SLOW_CONFIG)
		{
			best_match = i;
			continue;
		}

		eglGetConfigAttrib(g_display, config[best_match], EGL_BUFFER_SIZE, &value[0]);
		eglGetConfigAttrib(g_display, config[i], EGL_BUFFER_SIZE, &value[1]);

		if (value[0] < value[1])
			continue;

		eglGetConfigAttrib(g_display, config[best_match], EGL_ALPHA_SIZE, &value[0]);
		eglGetConfigAttrib(g_display, config[i], EGL_ALPHA_SIZE, &value[1]);

		if (value[0] < value[1])
			continue;

		eglGetConfigAttrib(g_display, config[best_match], EGL_DEPTH_SIZE, &value[0]);
		eglGetConfigAttrib(g_display, config[i], EGL_DEPTH_SIZE, &value[1]);

		if (value[0] < value[1])
			continue;

		eglGetConfigAttrib(g_display, config[best_match], EGL_SAMPLES, &value[0]);
		eglGetConfigAttrib(g_display, config[i], EGL_SAMPLES, &value[1]);

		if (value[0] < value[1])
			continue;

		best_match = i;
	}

	EGLint value;
	eglGetConfigAttrib(g_display, config[best_match], EGL_CONFIG_ID, &value);

	std::cout << "among " << num_config << " config candidate(s), best match is EGL_CONFIG_ID 0x" <<
		std::hex << std::setw(8) << std::setfill('0') << value << std::dec << "\n" << std::endl;

	return best_match;
}


static void
reportNativePixmap(
	const unsigned i)
{
	const unsigned last = sizeof(g_pixmap[i]) / sizeof(g_pixmap[i][0]) - 1;
	std::cout << std::hex;

	for (unsigned j = 0; j < last; ++j)
		std::cout << g_pixmap[i][j] << ", ";

	std::cout << g_pixmap[i][last] << std::dec << std::endl;
}


static bool
check_context(
	const char* prefix)
{
	bool context_correct = true;

	if (g_display != eglGetCurrentDisplay())
	{
		std::cerr << prefix << " encountered foreign display" << std::endl;

		context_correct = false;
	}

	if (g_context != eglGetCurrentContext())
	{
		std::cerr << prefix << " encountered foreign context" << std::endl;

		context_correct = false;
	}

	return context_correct;
}


bool
hook::deinit_resources()
{
	if (!check_context(__FUNCTION__))
		return false;

	for (unsigned i = 0; i < sizeof(g_shader_prog) / sizeof(g_shader_prog[0]); ++i)
	{
		glDeleteProgram(g_shader_prog[i]);
		g_shader_prog[i] = 0;
	}

	for (unsigned i = 0; i < sizeof(g_shader_vert) / sizeof(g_shader_vert[0]); ++i)
	{
		glDeleteShader(g_shader_vert[i]);
		g_shader_vert[i] = 0;
	}

	for (unsigned i = 0; i < sizeof(g_shader_frag) / sizeof(g_shader_frag[0]); ++i)
	{
		glDeleteShader(g_shader_frag[i]);
		g_shader_frag[i] = 0;
	}

	for (unsigned i = 0; i < sizeof(g_image) / sizeof(g_image[0]); ++i)
	{
		if (EGL_NO_IMAGE_KHR != g_image[i])
		{
			eglDestroyImageKHR(g_display, g_image[i]);
			g_image[i] = EGL_NO_IMAGE_KHR;
		}

		if (EGL_NO_SURFACE != g_surface[i])
		{
			eglDestroySurface(g_display, g_surface[i]);
			g_surface[i] = EGL_NO_SURFACE;
		}

		if (0 != g_pixmap[i][0])
		{
			eglDestroyGlobalImageBRCM(g_pixmap[i]);
			memset(g_pixmap + i, 0, sizeof(g_pixmap[i]));
		}
	}

	glDeleteTextures(sizeof(g_tex) / sizeof(g_tex[0]), g_tex);
	memset(g_tex, 0, sizeof(g_tex));

	glDeleteBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);
	memset(g_vbo, 0, sizeof(g_vbo));

	g_display = EGL_NO_DISPLAY;
	g_context = EGL_NO_CONTEXT;

	return true;
}


bool
hook::init_resources(
	const unsigned argc,
	const char* const * argv)
{
	if (!parse_cli(argc, argv))
		return false;

	g_display = eglGetCurrentDisplay();

	if (EGL_NO_DISPLAY == g_display)
	{
		std::cerr << __FUNCTION__ << " encountered nil display" << std::endl;
		return false;
	}

	g_context = eglGetCurrentContext();

	if (EGL_NO_CONTEXT == g_context)
	{
		std::cerr << __FUNCTION__ << " encountered nil context" << std::endl;
		return false;
	}

	scoped_ptr< deinit_resources_t, scoped_functor > on_error(deinit_resources);

	/////////////////////////////////////////////////////////////////

	glEnable(GL_CULL_FACE);

	/////////////////////////////////////////////////////////////////

	glGenTextures(sizeof(g_tex) / sizeof(g_tex[0]), g_tex);

	for (unsigned i = 0; i < sizeof(g_tex) / sizeof(g_tex[0]); ++i)
		assert(g_tex[i]);

	for (unsigned i = 0; i < num_images; ++i)
	{
#if 1
		// Take the round-trip to an EGLImage update through an EGLSurface:
		// 1. Create a native pixmap
		// 2. Create an EGLSurface from said native pixmap
		// 3. Create an EGLImage from said native pixmap
		// 4. Lock said EGLSurface to update said EGLImage
		const EGLint conf_attr[] =
		{
			EGL_DEPTH_SIZE,
			0,
#if 1
			EGL_RED_SIZE,
			5,
			EGL_GREEN_SIZE,
			6,
			EGL_BLUE_SIZE,
			5,
			EGL_ALPHA_SIZE,
			0,
#else
			EGL_MATCH_FORMAT_KHR,
			EGL_FORMAT_RGB_565_KHR,
#endif
			EGL_SURFACE_TYPE,
			EGL_PIXMAP_BIT,
			EGL_RENDERABLE_TYPE,
			EGL_OPENGL_ES2_BIT,
			EGL_NONE
		};

		EGLConfig config[256];
		EGLint num_config;

		if (EGL_FALSE == eglChooseConfig(g_display, conf_attr, config,
			sizeof(config) / sizeof(config[0]), &num_config))
		{
			std::cerr << __FUNCTION__ << " failed at eglChooseConfig" << std::endl;
			return false;
		}

		assert(0 < num_config);

		unsigned best_match = findOptimalConfig(config, num_config);
#endif
		EGLint pixel_format =
			EGL_PIXEL_FORMAT_RGB_565_BRCM |
			EGL_PIXEL_FORMAT_RENDER_GLES2_BRCM |
			EGL_PIXEL_FORMAT_GLES2_TEXTURE_BRCM;
#if 1
		EGLint value;
		eglGetConfigAttrib(g_display, config[best_match], EGL_RENDERABLE_TYPE, &value);

		if (value & EGL_OPENGL_ES_BIT)
		{
			pixel_format |= EGL_PIXEL_FORMAT_RENDER_GLES_BRCM;
			pixel_format |= EGL_PIXEL_FORMAT_GLES_TEXTURE_BRCM;
		}
		if (value & EGL_OPENVG_BIT)
		{
			pixel_format |= EGL_PIXEL_FORMAT_RENDER_VG_BRCM;
			pixel_format |= EGL_PIXEL_FORMAT_VG_IMAGE_BRCM;
		}
		if (value & EGL_OPENGL_BIT)
		{
			pixel_format |= EGL_PIXEL_FORMAT_RENDER_GL_BRCM;
		}
#endif
		eglCreateGlobalImageBRCM(
			g_image_w,
			g_image_h,
			pixel_format,
			0, /* const void *data */
			0, /* EGLint data_stride */
			g_pixmap[i]);

		if (util::reportEGLError())
		{
			std::cerr << __FUNCTION__ << " failed at eglCreateGlobalImageBRCM" << std::endl;
			return false;
		}

		assert(0 != g_pixmap[i][0]);
#if 0
		g_pixmap[i][2] = g_image_w;
		g_pixmap[i][3] = g_image_h;
		g_pixmap[i][4] = pixel_format;
#else
		if (EGL_FALSE == eglQueryGlobalImageBRCM(g_pixmap[i], g_pixmap[i] + 2))
		{
			std::cerr << __FUNCTION__ << " failed at eglQueryGlobalImageBRCM" << std::endl;
			util::reportEGLError();
			return false;
		}
#endif
#if 1
		g_surface[i] = eglCreatePixmapSurface(g_display,
			config[best_match], g_pixmap[i], NULL);

		if (EGL_NO_SURFACE == g_surface[i])
		{
			std::cerr << __FUNCTION__ << " failed at eglCreatePixmapSurface" << std::endl;
			util::reportEGLError();
			return false;
		}
#endif
		const EGLint image_attr[] =
		{
			EGL_IMAGE_PRESERVED_KHR, EGL_FALSE,
			EGL_NONE
		};

		g_image[i] = eglCreateImageKHR(g_display, EGL_NO_CONTEXT,
#if 0
			EGL_IMAGE_FROM_SURFACE_BRCM, (EGLClientBuffer) g_surface[i], image_attr
#else
			EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer) g_pixmap[i], image_attr
#endif
		);

		if (EGL_NO_IMAGE_KHR == g_image[i])
		{
			std::cerr << __FUNCTION__ << " failed at eglCreateImageKHR" << std::endl;
			util::reportEGLError();
			return false;
		}

		glBindTexture(GL_TEXTURE_2D, g_tex[i]);
		glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, g_image[i]);

		if (util::reportGLError())
		{
			std::cerr << __FUNCTION__ << " failed at glEGLImageTargetTexture2DOES" << std::endl;
			return false;
		}

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	/////////////////////////////////////////////////////////////////

	for (unsigned i = 0; i < PROG_COUNT; ++i)
		for (unsigned j = 0; j < UNI_COUNT; ++j)
			g_uni[i][j] = -1;

	/////////////////////////////////////////////////////////////////

	g_shader_vert[PROG_QUAD] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_QUAD]);

	if (!util::setupShader(g_shader_vert[PROG_QUAD], "texture.glslv"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_frag[PROG_QUAD] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_QUAD]);

	if (!util::setupShader(g_shader_frag[PROG_QUAD], "texture.glslf"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_prog[PROG_QUAD] = glCreateProgram();
	assert(g_shader_prog[PROG_QUAD]);

	if (!util::setupProgram(
			g_shader_prog[PROG_QUAD],
			g_shader_vert[PROG_QUAD],
			g_shader_frag[PROG_QUAD]))
	{
		std::cerr << __FUNCTION__ << " failed at setupProgram" << std::endl;
		return false;
	}

	g_uni[PROG_QUAD][UNI_SAMPLER_ALBEDO] = glGetUniformLocation(g_shader_prog[PROG_QUAD], "albedo_map");

	g_active_attr_semantics[PROG_QUAD].registerVertexAttr(
		glGetAttribLocation(g_shader_prog[PROG_QUAD], "at_Vertex"));
	g_active_attr_semantics[PROG_QUAD].registerTCoordAttr(
		glGetAttribLocation(g_shader_prog[PROG_QUAD], "at_MultiTexCoord0"));

	/////////////////////////////////////////////////////////////////

	glGenBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);

	for (unsigned i = 0; i < sizeof(g_vbo) / sizeof(g_vbo[0]); ++i)
		assert(g_vbo[i]);

	if (!createIndexedGridQuad(
			g_vbo[VBO_QUAD_VTX],
			g_vbo[VBO_QUAD_IDX],
			g_num_faces[MESH_QUAD]))
	{
		std::cerr << __FUNCTION__ << " failed at createIndexedQuad" << std::endl;
		return false;
	}

	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_QUAD_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_QUAD_IDX]);

	if (!setupVertexAttrPointers< Vertex >(g_active_attr_semantics[PROG_QUAD])
#if !defined(DEBUG)
		|| util::reportGLError()
#endif
		)
	{
		std::cerr << __FUNCTION__ <<
			" failed at setupVertexAttrPointers" << std::endl;
		return false;
	}

	on_error.reset();
	return true;
}


bool
hook::render_frame()
{
	if (!check_context(__FUNCTION__))
		return false;

	static unsigned idx;
	idx ^= 1;

	glUseProgram(g_shader_prog[PROG_QUAD]);

	DEBUG_GL_ERR()

	if (g_tex[idx] && -1 != g_uni[PROG_QUAD][UNI_SAMPLER_ALBEDO])
	{
		// No syncs/fences, so use a global wait
		eglWaitClient();

		static const EGLint attrib_list[] =
		{
			EGL_LOCK_USAGE_HINT_KHR, EGL_WRITE_SURFACE_BIT_KHR,
			EGL_NONE
		};

		if (EGL_FALSE == eglLockSurfaceKHR(g_display, g_surface[idx], attrib_list))
		{
			std::cerr << __FUNCTION__ << " failed at eglLockSurfaceKHR" << std::endl;
			util::reportEGLError();
			return false;
		}

		// update surface here

		if (EGL_FALSE == eglUnlockSurfaceKHR(g_display, g_surface[idx]))
		{
			std::cerr << __FUNCTION__ << " failed at eglUnlockSurfaceKHR" << std::endl;
			util::reportEGLError();
			return false;
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_tex[idx]);

		glUniform1i(g_uni[PROG_QUAD][UNI_SAMPLER_ALBEDO], 0);
	}

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_QUAD].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_QUAD].active_attr[i]);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_num_drawcalls; ++i)
		glDrawElements(GL_TRIANGLES, g_num_faces[MESH_QUAD] * 3, GL_UNSIGNED_SHORT, 0);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_QUAD].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_QUAD].active_attr[i]);

	DEBUG_GL_ERR()

	return true;
}

} // namespace testbed
