#if defined(PLATFORM_GLX)

#include <GL/gl.h>

#else

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "pure_macro.hpp"

#endif // PLATFORM_GLX

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <sstream>

#include "utilTex.hpp"
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

static const char arg_albedo[]	= "albedo_map";
static const char arg_nearest[] = "nearest";

static char g_albedo_filename[FILENAME_MAX + 1] = "graph_paper.raw";
static unsigned g_albedo_w = 512;
static unsigned g_albedo_h = 512;

static bool g_nearest;

#if !defined(PLATFORM_GLX)

static EGLDisplay g_display = EGL_NO_DISPLAY;
static EGLContext g_context = EGL_NO_CONTEXT;

#endif

enum {
	TEX_ALBEDO,

	TEX_COUNT,
	TEX_FORCE_UINT = -1U
};

enum {
	PROG_INSPECTOR,

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

#if defined(PLATFORM_GLX)

static GLuint g_vao[PROG_COUNT];

#endif

static GLuint g_tex[TEX_COUNT];
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
				if (!strcmp(option, arg_albedo))
					if (3 == sscanf(argv[i] + opt_arg_start, "%" XQUOTE(FILENAME_MAX) "s %u %u",
								g_albedo_filename, &g_albedo_w, &g_albedo_h))
					{
						continue;
					}

				if (!strcmp(option, arg_nearest))
				{
					g_nearest = true;
					continue;
				}
			}
		}

		cli_err = true;
	}

	if (cli_err)
	{
		std::cerr << "app options (multiple args to an option must constitute a single string, eg. -foo \"a b c\"):\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_albedo <<
			" <filename> <width> <height>\t: use specified raw file and dimensions as source of albedo map\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_nearest <<
			"\t\t\t\t\t: use nearest filtering with the albedo map sampler\n" << std::endl;
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

	// tile texture mapping the following number of times:
	const float tile = 1.f;

	assert(rows > 1);
	assert(cols > 1);

	static Vertex arr[rows * cols];
	unsigned ai = 0;

	num_faces = (rows - 1) * (cols - 1) * 2;

	for (int i = 0; i < rows; ++i)
		for (int j = 0; j < cols; ++j)
		{
			assert(ai < sizeof(arr) / sizeof(arr[0]));

			arr[ai].pos[0] = -1.f + j * (2.f / (cols - 1));
			arr[ai].pos[1] =  1.f - i * (2.f / (rows - 1));
			arr[ai].pos[2] = 0.f;
			arr[ai].txc[0] = tile * j / (cols - 1);
			arr[ai].txc[1] = tile * i / (rows - 1);

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

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_idx);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

	return true;
}


template < unsigned PROG_T >
inline bool
bindVertexBuffersAndPointers()
{
	assert(false);

	return false;
}


template <>
inline bool
bindVertexBuffersAndPointers< PROG_INSPECTOR >()
{
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_QUAD_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_QUAD_IDX]);

	DEBUG_GL_ERR()

	return setupVertexAttrPointers< Vertex >(g_active_attr_semantics[PROG_INSPECTOR]);
}


static bool
check_context(
	const char* prefix)
{
	bool context_correct = true;

#if !defined(PLATFORM_GLX)

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

#endif

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

	glDeleteTextures(sizeof(g_tex) / sizeof(g_tex[0]), g_tex);
	memset(g_tex, 0, sizeof(g_tex));

#if defined(PLATFORM_GLX)

	glDeleteVertexArrays(sizeof(g_vao) / sizeof(g_vao[0]), g_vao);
	memset(g_vao, 0, sizeof(g_vao));

#endif

	glDeleteBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);
	memset(g_vbo, 0, sizeof(g_vbo));

#if !defined(PLATFORM_GLX)

	g_display = EGL_NO_DISPLAY;
	g_context = EGL_NO_CONTEXT;

#endif

	return true;
}


bool
hook::init_resources(
	const unsigned argc,
	const char* const * argv)
{
	if (!parse_cli(argc, argv))
		return false;

#if !defined(PLATFORM_GLX)

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

#endif

	scoped_ptr< deinit_resources_t, scoped_functor > on_error(deinit_resources);

	/////////////////////////////////////////////////////////////////

	glEnable(GL_CULL_FACE);

	/////////////////////////////////////////////////////////////////

	glGenTextures(sizeof(g_tex) / sizeof(g_tex[0]), g_tex);

	for (unsigned i = 0; i < sizeof(g_tex) / sizeof(g_tex[0]); ++i)
		assert(g_tex[i]);

	if (!util::setupTexture2D(g_tex[TEX_ALBEDO], g_albedo_filename, g_albedo_w, g_albedo_h))
	{
		std::cerr << __FUNCTION__ << " failed at setupTexture2D" << std::endl;
		return false;
	}

	if (g_albedo_w != g_albedo_h)
	{
		std::cerr << __FUNCTION__ << " failed obtaining equal texture width and height" << std::endl;
		return false;
	}

	if (g_nearest)
	{
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_ALBEDO]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	std::ostringstream patch_res;
	patch_res << "const float albedo_res = " << g_albedo_w << ".0;";

	const std::string patch[] =
	{
		// albedo map resolution patch
		"const float albedo_res = 2.0;",
		patch_res.str(),
		// nearest sampling patch
		"vec2 st = floor(uv) - 0.5 + step(0.5, fract(uv));",
		"vec2 st = uv - 0.5;"
	};

	const size_t patch_count = g_nearest
		? sizeof(patch) / sizeof(patch[0]) / 2
		: sizeof(patch) / sizeof(patch[0]) / 2 - 1;

	/////////////////////////////////////////////////////////////////

	for (unsigned i = 0; i < PROG_COUNT; ++i)
		for (unsigned j = 0; j < UNI_COUNT; ++j)
			g_uni[i][j] = -1;

	/////////////////////////////////////////////////////////////////

	g_shader_vert[PROG_INSPECTOR] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_INSPECTOR]);

	if (!util::setupShader(g_shader_vert[PROG_INSPECTOR], "texture.glslv"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_frag[PROG_INSPECTOR] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_INSPECTOR]);

	if (!util::setupShaderWithPatch(g_shader_frag[PROG_INSPECTOR], "linear.glslf",
			patch_count,
			patch))
	{
		std::cerr << __FUNCTION__ << " failed at setupShaderWithPatch" << std::endl;
		return false;
	}

	g_shader_prog[PROG_INSPECTOR] = glCreateProgram();
	assert(g_shader_prog[PROG_INSPECTOR]);

	if (!util::setupProgram(
			g_shader_prog[PROG_INSPECTOR],
			g_shader_vert[PROG_INSPECTOR],
			g_shader_frag[PROG_INSPECTOR]))
	{
		std::cerr << __FUNCTION__ << " failed at setupProgram" << std::endl;
		return false;
	}

	g_uni[PROG_INSPECTOR][UNI_SAMPLER_ALBEDO] =
		glGetUniformLocation(g_shader_prog[PROG_INSPECTOR], "albedo_map");

	g_active_attr_semantics[PROG_INSPECTOR].registerVertexAttr(
		glGetAttribLocation(g_shader_prog[PROG_INSPECTOR], "at_Vertex"));
	g_active_attr_semantics[PROG_INSPECTOR].registerTCoordAttr(
		glGetAttribLocation(g_shader_prog[PROG_INSPECTOR], "at_MultiTexCoord0"));

	/////////////////////////////////////////////////////////////////

#if defined(PLATFORM_GLX)

	glGenVertexArrays(sizeof(g_vao) / sizeof(g_vao[0]), g_vao);

	for (unsigned i = 0; i < sizeof(g_vao) / sizeof(g_vao[0]); ++i)
		assert(g_vao[i]);

#endif

	glGenBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);

	for (unsigned i = 0; i < sizeof(g_vbo) / sizeof(g_vbo[0]); ++i)
		assert(g_vbo[i]);

	createIndexedGridQuad(
		g_vbo[VBO_QUAD_VTX],
		g_vbo[VBO_QUAD_IDX],
		g_num_faces[MESH_QUAD]);

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_INSPECTOR]);

	if (!bindVertexBuffersAndPointers< PROG_INSPECTOR >()
#if !defined(DEBUG)
		|| util::reportGLError()
#endif
		)
	{
		std::cerr << __FUNCTION__ <<
			"failed at bindVertexBuffersAndPointers for PROG_INSPECTOR" << std::endl;
		return false;
	}

	glBindVertexArray(0);

#endif

	/////////////////////////////////////////////////////////////////

	on_error.reset();
	return true;
}


bool
hook::render_frame()
{
	if (!check_context(__FUNCTION__))
		return false;

	glUseProgram(g_shader_prog[PROG_INSPECTOR]);

	DEBUG_GL_ERR()

	if (0 != g_tex[TEX_ALBEDO] && -1 != g_uni[PROG_INSPECTOR][UNI_SAMPLER_ALBEDO])
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_ALBEDO]);

		glUniform1i(g_uni[PROG_INSPECTOR][UNI_SAMPLER_ALBEDO], 0);
	}

	DEBUG_GL_ERR()

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_INSPECTOR]);

	DEBUG_GL_ERR()

#else

	if (!bindVertexBuffersAndPointers< PROG_INSPECTOR >())
		return false;

#endif

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_INSPECTOR].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_INSPECTOR].active_attr[i]);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_num_drawcalls; ++i)
		glDrawElements(GL_TRIANGLES, g_num_faces[MESH_QUAD] * 3, GL_UNSIGNED_SHORT, 0);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_INSPECTOR].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_INSPECTOR].active_attr[i]);

	DEBUG_GL_ERR()

	return true;
}

} // namespace testbed
