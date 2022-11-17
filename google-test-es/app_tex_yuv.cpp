#if defined(PLATFORM_GLX)

#include <GL/gl.h>

#else

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#endif

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <iostream>

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

static const char arg_albedo[] = "albedo_map";

static char g_albedo_filename[FILENAME_MAX + 1] = "rockwall.raw";
static unsigned g_albedo_w = 256;
static unsigned g_albedo_h = 256;

#if !defined(PLATFORM_GLX)

static EGLDisplay g_display = EGL_NO_DISPLAY;
static EGLContext g_context = EGL_NO_CONTEXT;

#endif

enum {
	TEX_ALBEDO_Y,
	TEX_ALBEDO_U,
	TEX_ALBEDO_V,

	TEX_COUNT,
	TEX_FORCE_UINT = -1U
};

enum {
	PROG_TEX_YUV,

	PROG_COUNT,
	PROG_FORCE_UINT = -1U
};

enum {
	UNI_SAMPLER_ALBEDO_Y,
	UNI_SAMPLER_ALBEDO_U,
	UNI_SAMPLER_ALBEDO_V,

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
			}
		}

		cli_err = true;
	}

	if (cli_err)
	{
		std::cerr << "app options (multiple args to an option must constitute a single string, eg. -foo \"a b c\"):\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_albedo <<
			" <filename> <width> <height>\t: use specified raw file and dimensions as source of albedo map\n" << std::endl;
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

	num_faces = (rows - 1) * (cols - 1) * 2;

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

	if (!util::setupTextureYUV420(g_tex, g_albedo_filename, g_albedo_w, g_albedo_h))
	{
		std::cerr << __FUNCTION__ << " at setupTextureYUV420" << std::endl;
		return false;
	}

	/////////////////////////////////////////////////////////////////

	for (unsigned i = 0; i < PROG_COUNT; ++i)
		for (unsigned j = 0; j < UNI_COUNT; ++j)
			g_uni[i][j] = -1;

	/////////////////////////////////////////////////////////////////

	g_shader_vert[PROG_TEX_YUV] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_TEX_YUV]);

	if (!util::setupShader(g_shader_vert[PROG_TEX_YUV], "texture.glslv"))
	{
		std::cerr << __FUNCTION__ << " at setupShader" << std::endl;
		return false;
	}

	g_shader_frag[PROG_TEX_YUV] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_TEX_YUV]);

	if (!util::setupShader(g_shader_frag[PROG_TEX_YUV], "texture_yuv420.glslf"))
	{
		std::cerr << __FUNCTION__ << " at setupShader" << std::endl;
		return false;
	}

	g_shader_prog[PROG_TEX_YUV] = glCreateProgram();
	assert(g_shader_prog[PROG_TEX_YUV]);

	if (!util::setupProgram(
			g_shader_prog[PROG_TEX_YUV],
			g_shader_vert[PROG_TEX_YUV],
			g_shader_frag[PROG_TEX_YUV]))
	{
		std::cerr << __FUNCTION__ << " at setupProgram" << std::endl;
		return false;
	}

	g_uni[PROG_TEX_YUV][UNI_SAMPLER_ALBEDO_Y] = glGetUniformLocation(g_shader_prog[PROG_TEX_YUV], "y_map");
	g_uni[PROG_TEX_YUV][UNI_SAMPLER_ALBEDO_U] = glGetUniformLocation(g_shader_prog[PROG_TEX_YUV], "u_map");
	g_uni[PROG_TEX_YUV][UNI_SAMPLER_ALBEDO_V] = glGetUniformLocation(g_shader_prog[PROG_TEX_YUV], "v_map");

	g_active_attr_semantics[PROG_TEX_YUV].registerVertexAttr(glGetAttribLocation(g_shader_prog[PROG_TEX_YUV], "at_Vertex"));
	g_active_attr_semantics[PROG_TEX_YUV].registerTCoordAttr(glGetAttribLocation(g_shader_prog[PROG_TEX_YUV], "at_MultiTexCoord0"));

	/////////////////////////////////////////////////////////////////

#if defined(PLATFORM_GLX)

	glGenVertexArrays(sizeof(g_vao) / sizeof(g_vao[0]), g_vao);

	for (unsigned i = 0; i < sizeof(g_vao) / sizeof(g_vao[0]); ++i)
		assert(g_vao[i]);

#endif

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

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_TEX_YUV]);

#endif

	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_QUAD_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_QUAD_IDX]);

	if (!setupVertexAttrPointers< Vertex >(g_active_attr_semantics[PROG_TEX_YUV])
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

	glUseProgram(g_shader_prog[PROG_TEX_YUV]);

	DEBUG_GL_ERR()

	if (g_tex[TEX_ALBEDO_Y] && -1 != g_uni[PROG_TEX_YUV][UNI_SAMPLER_ALBEDO_Y])
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_ALBEDO_Y]);

		glUniform1i(g_uni[PROG_TEX_YUV][UNI_SAMPLER_ALBEDO_Y], 0);
	}

	DEBUG_GL_ERR()

	if (g_tex[TEX_ALBEDO_U] && -1 != g_uni[PROG_TEX_YUV][UNI_SAMPLER_ALBEDO_U])
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_ALBEDO_U]);

		glUniform1i(g_uni[PROG_TEX_YUV][UNI_SAMPLER_ALBEDO_U], 1);
	}

	DEBUG_GL_ERR()

	if (g_tex[TEX_ALBEDO_V] && -1 != g_uni[PROG_TEX_YUV][UNI_SAMPLER_ALBEDO_V])
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_ALBEDO_V]);

		glUniform1i(g_uni[PROG_TEX_YUV][UNI_SAMPLER_ALBEDO_V], 2);
	}

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_TEX_YUV].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_TEX_YUV].active_attr[i]);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_num_drawcalls; ++i)
		glDrawElements(GL_TRIANGLES, g_num_faces[MESH_QUAD] * 3, GL_UNSIGNED_SHORT, 0);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_TEX_YUV].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_TEX_YUV].active_attr[i]);

	DEBUG_GL_ERR()

	return true;
}

} // namespace testbed
