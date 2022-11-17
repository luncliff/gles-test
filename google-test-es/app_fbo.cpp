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
#include <math.h>
#include <string>
#include <iostream>

#include "rendVect.hpp"
#include "utilTex.hpp"
#include "testbed.hpp"

#include "rendVertAttr.hpp"

namespace sp
{

#define SETUP_VERTEX_ATTR_POINTERS_MASK	(			\
		SETUP_VERTEX_ATTR_POINTERS_MASK_vertex |	\
		SETUP_VERTEX_ATTR_POINTERS_MASK_normal |	\
		SETUP_VERTEX_ATTR_POINTERS_MASK_tcoord)

#include "rendVertAttr_setupVertAttrPointers.hpp"
#undef SETUP_VERTEX_ATTR_POINTERS_MASK

struct Vertex
{
	GLfloat pos[3];
	GLfloat nrm[3];
	GLfloat txc[2];
};

} // namespace sp

namespace qu
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

} // namespace qu

namespace testbed
{

const GLsizei fbo_w = 256;
const GLsizei fbo_h = 256;

#define OPTION_IDENTIFIER_MAX	64

static const char arg_normal[] = "normal_map";
static const char arg_albedo[] = "albedo_map";
static const char arg_tile[] = "tile";

static char g_normal_filename[FILENAME_MAX + 1] = "rockwall_NH.raw";
static unsigned g_normal_w = 64;
static unsigned g_normal_h = 64;

static char g_albedo_filename[FILENAME_MAX + 1] = "rockwall.raw";
static unsigned g_albedo_w = 256;
static unsigned g_albedo_h = 256;

static float g_tile = 2.f;

#if !defined(PLATFORM_GLX)

static EGLDisplay g_display = EGL_NO_DISPLAY;
static EGLContext g_context = EGL_NO_CONTEXT;

#endif

enum {
	TEX_NORMAL,
	TEX_ALBEDO,
	TEX_FBO,

	TEX_COUNT,
	TEX_FORCE_UINT = -1U
};

enum {
	PROG_SPHERE,
	PROG_FBO,

	PROG_COUNT,
	PROG_FORCE_UINT = -1U
};

enum {
	UNI_SAMPLER_NORMAL,
	UNI_SAMPLER_ALBEDO,
	UNI_SAMPLER_FBO,

	UNI_LP_OBJ,
	UNI_VP_OBJ,
	UNI_MVP,

	UNI_COUNT,
	UNI_FORCE_UINT = -1U
};

enum {
	MESH_SPHERE,
	MESH_QUAD,

	MESH_COUNT,
	MESH_FORCE_UINT = -1U
};

enum {
	VBO_SPHERE_VTX,
	VBO_SPHERE_IDX,
	VBO_QUAD_VTX,
	VBO_QUAD_IDX,

	VBO_COUNT,
	VBO_FORCE_UINT = -1U
};

static GLint g_uni[PROG_COUNT][UNI_COUNT];

#if defined(PLATFORM_GLX)

static GLuint g_vao[PROG_COUNT];

#endif

static GLuint g_fbo;
static GLuint g_tex[TEX_COUNT];
static GLuint g_vbo[VBO_COUNT];
static GLuint g_shader_vert[PROG_COUNT];
static GLuint g_shader_frag[PROG_COUNT];
static GLuint g_shader_prog[PROG_COUNT];

static GLint g_vport[4];
static unsigned g_num_faces[MESH_COUNT];

static rend::ActiveAttrSemantics g_active_attr_semantics[PROG_COUNT];


bool
hook::set_num_drawcalls(
	const unsigned)
{
	return false;
}


unsigned
hook::get_num_drawcalls()
{
	return 1;
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
				if (!strcmp(option, arg_normal))
					if (3 == sscanf(argv[i] + opt_arg_start, "%" XQUOTE(FILENAME_MAX) "s %u %u",
								g_normal_filename, &g_normal_w, &g_normal_h))
					{
						continue;
					}

				if (!strcmp(option, arg_albedo))
					if (3 == sscanf(argv[i] + opt_arg_start, "%" XQUOTE(FILENAME_MAX) "s %u %u",
								g_albedo_filename, &g_albedo_w, &g_albedo_h))
					{
						continue;
					}

				if (!strcmp(option, arg_tile))
					if (1 == sscanf(argv[i] + opt_arg_start, "%f", &g_tile) && 0.f < g_tile)
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
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_normal <<
			" <filename> <width> <height>\t: use specified raw file and dimensions as source of normal map\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_albedo <<
			" <filename> <width> <height>\t: use specified raw file and dimensions as source of albedo map\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_tile <<
			" <n>\t\t\t\t\t: tile texture maps the specified number of times along U, half as much along V\n" << std::endl;
	}

	return !cli_err;
}


static bool
createIndexedPolarSphere(
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	unsigned& num_faces)
{
	assert(vbo_arr && vbo_idx);

	// bend a polar sphere from a grid of the following dimensions:
	const int rows = 33;
	const int cols = 65;

	assert(rows > 2);
	assert(cols > 3);

	static sp::Vertex arr[(rows - 2) * cols + 2 * (cols - 1)];
	unsigned ai = 0;

	num_faces = ((rows - 3) * 2 + 2) * (cols - 1);

	const float r = 1.f;

	// north pole
	for (int j = 0; j < cols - 1; ++j)
	{
		assert(ai < sizeof(arr) / sizeof(arr[0]));

		arr[ai].pos[0] = 0.f;
		arr[ai].pos[1] = r;
		arr[ai].pos[2] = 0.f;

		arr[ai].nrm[0] = 0.f;
		arr[ai].nrm[1] = 1.f;
		arr[ai].nrm[2] = 0.f;

		arr[ai].txc[0] = g_tile * (j + .5f) / (cols - 1);
		arr[ai].txc[1] = g_tile * .5f;

		++ai;
	}

	for (int i = 1; i < rows - 1; ++i)
		for (int j = 0; j < cols; ++j)
		{
			assert(ai < sizeof(arr) / sizeof(arr[0]));

			const rend::matx3& azim =
				rend::matx3().rotate((j * 2) * M_PI / (cols - 1), 0.f, 1.f, 0.f);

			const rend::matx3& decl =
				rend::matx3().rotate(- M_PI_2 + M_PI * i / (rows - 1), 1.f, 0.f, 0.f);

			const rend::matx3& azim_decl = rend::matx3().mul(azim, decl);

			arr[ai].pos[0] = azim_decl[0][2] * r;
			arr[ai].pos[1] = azim_decl[1][2] * r;
			arr[ai].pos[2] = azim_decl[2][2] * r;

			arr[ai].nrm[0] = azim_decl[0][2];
			arr[ai].nrm[1] = azim_decl[1][2];
			arr[ai].nrm[2] = azim_decl[2][2];

			arr[ai].txc[0] = g_tile * j / (cols - 1);
			arr[ai].txc[1] = g_tile * (rows - 1 - i) / (2 * rows - 2);

			++ai;
		}

	// south pole
	for (int j = 0; j < cols - 1; ++j)
	{
		assert(ai < sizeof(arr) / sizeof(arr[0]));

		arr[ai].pos[0] = 0.f;
		arr[ai].pos[1] = -r;
		arr[ai].pos[2] = 0.f;

		arr[ai].nrm[0] = 0.f;
		arr[ai].nrm[1] = -1.f;
		arr[ai].nrm[2] = 0.f;

		arr[ai].txc[0] = g_tile * (j + .5f) / (cols - 1);
		arr[ai].txc[1] = 0.f;

		++ai;
	}

	assert(ai == sizeof(arr) / sizeof(arr[0]));

	static uint16_t idx[((rows - 3) * 2 + 2) * (cols - 1)][3];
	unsigned ii = 0;

	// north pole
	for (unsigned j = 0; j < cols - 1; ++j)
	{
		idx[ii][0] = (uint16_t)(j);
		idx[ii][1] = (uint16_t)(j + cols - 1);
		idx[ii][2] = (uint16_t)(j + cols);

		++ii;
	}

	for (int i = 1; i < rows - 2; ++i)
		for (int j = 0; j < cols - 1; ++j)
		{
			idx[ii][0] = (uint16_t)(j + i * cols);
			idx[ii][1] = (uint16_t)(j + i * cols - 1);
			idx[ii][2] = (uint16_t)(j + (i + 1) * cols);

			++ii;

			idx[ii][0] = (uint16_t)(j + (i + 1) * cols - 1);
			idx[ii][1] = (uint16_t)(j + (i + 1) * cols);
			idx[ii][2] = (uint16_t)(j + i * cols - 1);

			++ii;
		}

	// south pole
	for (unsigned j = 0; j < cols - 1; ++j)
	{
		idx[ii][0] = (uint16_t)(j + (rows - 2) * cols);
		idx[ii][1] = (uint16_t)(j + (rows - 2) * cols - 1);
		idx[ii][2] = (uint16_t)(j + (rows - 2) * cols + cols - 1);

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
	const float tile = 2.f;

	assert(rows > 1);
	assert(cols > 1);

	static qu::Vertex arr[rows * cols];
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


template < unsigned PROG_T >
inline bool
bindVertexBuffersAndPointers()
{
	assert(false);

	return false;
}


template <>
inline bool
bindVertexBuffersAndPointers< PROG_SPHERE >()
{
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_SPHERE_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_SPHERE_IDX]);

	DEBUG_GL_ERR()

	return sp::setupVertexAttrPointers< sp::Vertex >(g_active_attr_semantics[PROG_SPHERE]);
}


template <>
inline bool
bindVertexBuffersAndPointers< PROG_FBO >()
{
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_QUAD_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_QUAD_IDX]);

	DEBUG_GL_ERR()

	return qu::setupVertexAttrPointers< qu::Vertex >(g_active_attr_semantics[PROG_FBO]);
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

	glDeleteFramebuffers(1, &g_fbo);
	g_fbo = 0;

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

	glGetIntegerv(GL_VIEWPORT, g_vport);

	const GLclampf red = 0.f;
	const GLclampf green = 0.f;
	const GLclampf blue = 0.f;
	const GLclampf alpha = 1.f;

	glClearColor(
		red, 
		green, 
		blue, 
		alpha);

	/////////////////////////////////////////////////////////////////

	glGenTextures(sizeof(g_tex) / sizeof(g_tex[0]), g_tex);

	for (unsigned i = 0; i < sizeof(g_tex) / sizeof(g_tex[0]); ++i)
		assert(g_tex[i]);

	if (!util::setupTexture2D(g_tex[TEX_NORMAL], g_normal_filename, g_normal_w, g_normal_h))
	{
		std::cerr << __FUNCTION__ << " failed at setupTexture2D" << std::endl;
		return false;
	}

	if (!util::setupTexture2D(g_tex[TEX_ALBEDO], g_albedo_filename, g_albedo_w, g_albedo_h))
	{
		std::cerr << __FUNCTION__ << " failed at setupTexture2D" << std::endl;
		return false;
	}

	/////////////////////////////////////////////////////////////////

	for (unsigned i = 0; i < PROG_COUNT; ++i)
		for (unsigned j = 0; j < UNI_COUNT; ++j)
			g_uni[i][j] = -1;

	/////////////////////////////////////////////////////////////////

	g_shader_vert[PROG_SPHERE] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_SPHERE]);

	if (!util::setupShader(g_shader_vert[PROG_SPHERE], "phong_bump_tang.glslv"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_frag[PROG_SPHERE] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_SPHERE]);

	if (!util::setupShader(g_shader_frag[PROG_SPHERE], "phong_bump_tang.glslf"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_prog[PROG_SPHERE] = glCreateProgram();
	assert(g_shader_prog[PROG_SPHERE]);

	if (!util::setupProgram(
			g_shader_prog[PROG_SPHERE],
			g_shader_vert[PROG_SPHERE],
			g_shader_frag[PROG_SPHERE]))
	{
		std::cerr << __FUNCTION__ << " failed at setupProgram" << std::endl;
		return false;
	}

	g_uni[PROG_SPHERE][UNI_MVP]		= glGetUniformLocation(g_shader_prog[PROG_SPHERE], "mvp");
	g_uni[PROG_SPHERE][UNI_LP_OBJ]	= glGetUniformLocation(g_shader_prog[PROG_SPHERE], "lp_obj");
	g_uni[PROG_SPHERE][UNI_VP_OBJ]	= glGetUniformLocation(g_shader_prog[PROG_SPHERE], "vp_obj");

	g_uni[PROG_SPHERE][UNI_SAMPLER_NORMAL] = glGetUniformLocation(g_shader_prog[PROG_SPHERE], "normal_map");
	g_uni[PROG_SPHERE][UNI_SAMPLER_ALBEDO] = glGetUniformLocation(g_shader_prog[PROG_SPHERE], "albedo_map");

	g_active_attr_semantics[PROG_SPHERE].registerVertexAttr(
		glGetAttribLocation(g_shader_prog[PROG_SPHERE], "at_Vertex"));
	g_active_attr_semantics[PROG_SPHERE].registerNormalAttr(
		glGetAttribLocation(g_shader_prog[PROG_SPHERE], "at_Normal"));
	g_active_attr_semantics[PROG_SPHERE].registerTCoordAttr(
		glGetAttribLocation(g_shader_prog[PROG_SPHERE], "at_MultiTexCoord0"));

	g_shader_vert[PROG_FBO] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_FBO]);

	if (!util::setupShader(g_shader_vert[PROG_FBO], "texture.glslv"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_frag[PROG_FBO] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_FBO]);

	if (!util::setupShader(g_shader_frag[PROG_FBO], "texture.glslf"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_prog[PROG_FBO] = glCreateProgram();
	assert(g_shader_prog[PROG_FBO]);

	if (!util::setupProgram(
			g_shader_prog[PROG_FBO],
			g_shader_vert[PROG_FBO],
			g_shader_frag[PROG_FBO]))
	{
		std::cerr << __FUNCTION__ << " failed at setupProgram" << std::endl;
		return false;
	}

	g_uni[PROG_FBO][UNI_SAMPLER_FBO] = glGetUniformLocation(g_shader_prog[PROG_FBO], "albedo_map");

	g_active_attr_semantics[PROG_FBO].registerVertexAttr(
		glGetAttribLocation(g_shader_prog[PROG_FBO], "at_Vertex"));
	g_active_attr_semantics[PROG_FBO].registerTCoordAttr(
		glGetAttribLocation(g_shader_prog[PROG_FBO], "at_MultiTexCoord0"));

	/////////////////////////////////////////////////////////////////

#if defined(PLATFORM_GLX)

	glGenVertexArrays(sizeof(g_vao) / sizeof(g_vao[0]), g_vao);

	for (unsigned i = 0; i < sizeof(g_vao) / sizeof(g_vao[0]); ++i)
		assert(g_vao[i]);

#endif

	glGenBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);

	for (unsigned i = 0; i < sizeof(g_vbo) / sizeof(g_vbo[0]); ++i)
		assert(g_vbo[i]);

	if (!createIndexedPolarSphere(
			g_vbo[VBO_SPHERE_VTX],
			g_vbo[VBO_SPHERE_IDX],
			g_num_faces[MESH_SPHERE]))
	{
		std::cerr << __FUNCTION__ << " failed at createIndexedPolarSphere" << std::endl;
		return false;
	}

	if (!createIndexedGridQuad(
			g_vbo[VBO_QUAD_VTX],
			g_vbo[VBO_QUAD_IDX],
			g_num_faces[MESH_QUAD]))
	{
		std::cerr << __FUNCTION__ << " failed at createIndexedQuad" << std::endl;
		return false;
	}

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_SPHERE]);

	if (!bindVertexBuffersAndPointers< PROG_SPHERE >()
#if !defined(DEBUG)
		|| util::reportGLError()
#endif
		)
	{
		std::cerr << __FUNCTION__ <<
			" failed at bindVertexBuffersAndPointers for PROG_SPHERE" << std::endl;
		return false;
	}

	glBindVertexArray(g_vao[PROG_FBO]);

	if (!bindVertexBuffersAndPointers< PROG_FBO >()
#if !defined(DEBUG)
		|| util::reportGLError()
#endif
		)
	{
		std::cerr << __FUNCTION__ <<
			" failed at bindVertexBuffersAndPointers for PROG_FBO" << std::endl;
		return false;
	}

	glBindVertexArray(0);

#endif

	/////////////////////////////////////////////////////////////////

	glGenFramebuffers(1, &g_fbo);
	assert(g_fbo);

	glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);

	glBindTexture(GL_TEXTURE_2D, g_tex[TEX_FBO]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fbo_w, fbo_h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);

	if (util::reportGLError())
	{
		std::cerr << __FUNCTION__ << " failed at TEX_FBO creation" << std::endl;
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_tex[TEX_FBO], 0);

	const GLenum fbo_success = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (GL_FRAMEBUFFER_COMPLETE != fbo_success)
	{
		std::cerr << __FUNCTION__ << " failed at glCheckFramebufferStatus" << std::endl;
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

	glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
	glViewport(0, 0, fbo_w, fbo_h);

	DEBUG_GL_ERR()

	glFrontFace(GL_CW);

	glClear(GL_COLOR_BUFFER_BIT);

	/////////////////////////////////////////////////////////////////

	static GLfloat angle = 0.f;
	const GLfloat angleStep = 3.f / 40.f;

	const rend::matx3& r0 = rend::matx3().rotate(angle, 1.f, 0.f, 0.f);
	const rend::matx3& r1 = rend::matx3().rotate(angle, 0.f, 1.f, 0.f);
	const rend::matx3& r2 = rend::matx3().rotate(angle, 0.f, 0.f, 1.f);

	const rend::matx3& p0 = rend::matx3().mul(r1, r0);
	const rend::matx3& p1 = rend::matx3().mul(r2, p0);

	const GLfloat mvp[4][4] = // transpose and expand to 4x4, sign-inverting z in all original columns (for GL screen space)
	{
		{ p1[0][0], -p1[1][0], -p1[2][0], 0.f },
		{ p1[0][1], -p1[1][1], -p1[2][1], 0.f },
		{ p1[0][2], -p1[1][2], -p1[2][2], 0.f },
		{ 0.f,		0.f,	  0.f,		 1.f }
	};

	angle = fmodf(angle + angleStep, 2.f * M_PI);

	/////////////////////////////////////////////////////////////////

	glUseProgram(g_shader_prog[PROG_SPHERE]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SPHERE][UNI_MVP])
	{
		glUniformMatrix4fv(g_uni[PROG_SPHERE][UNI_MVP],
			1, GL_FALSE, reinterpret_cast< const GLfloat* >(mvp));
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SPHERE][UNI_LP_OBJ])
	{
		const GLfloat nonlocal_light[4] =
		{
			p1[2][0],
			p1[2][1],
			p1[2][2],
			0.f
		};

		glUniform4fv(g_uni[PROG_SPHERE][UNI_LP_OBJ], 1, nonlocal_light);
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SPHERE][UNI_VP_OBJ])
	{
		const GLfloat nonlocal_viewer[4] =
		{
			p1[2][0],
			p1[2][1],
			p1[2][2],
			0.f
		};

		glUniform4fv(g_uni[PROG_SPHERE][UNI_VP_OBJ], 1, nonlocal_viewer);
	}

	DEBUG_GL_ERR()

	if (g_tex[TEX_NORMAL] && -1 != g_uni[PROG_SPHERE][UNI_SAMPLER_NORMAL])
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_NORMAL]);

		glUniform1i(g_uni[PROG_SPHERE][UNI_SAMPLER_NORMAL], 0);
	}

	DEBUG_GL_ERR()

	if (g_tex[TEX_ALBEDO] && -1 != g_uni[PROG_SPHERE][UNI_SAMPLER_ALBEDO])
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_ALBEDO]);

		glUniform1i(g_uni[PROG_SPHERE][UNI_SAMPLER_ALBEDO], 1);
	}

	DEBUG_GL_ERR()

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_SPHERE]);

	DEBUG_GL_ERR()

#else

	if (!bindVertexBuffersAndPointers< PROG_SPHERE >())
		return false;

#endif

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SPHERE].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SPHERE].active_attr[i]);

	DEBUG_GL_ERR()

	glDrawElements(GL_TRIANGLES, g_num_faces[MESH_SPHERE] * 3, GL_UNSIGNED_SHORT, 0);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SPHERE].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_SPHERE].active_attr[i]);

	DEBUG_GL_ERR()

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(g_vport[0], g_vport[1], g_vport[2], g_vport[3]);

	DEBUG_GL_ERR()

	glFrontFace(GL_CCW);

	glUseProgram(g_shader_prog[PROG_FBO]);

	DEBUG_GL_ERR()

	if (g_tex[TEX_FBO] && -1 != g_uni[PROG_FBO][UNI_SAMPLER_FBO])
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_FBO]);

		glUniform1i(g_uni[PROG_FBO][UNI_SAMPLER_FBO], 0);
	}

	DEBUG_GL_ERR()

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_FBO]);

	DEBUG_GL_ERR()

#else

	if (!bindVertexBuffersAndPointers< PROG_FBO >())
		return false;

#endif

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_FBO].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_FBO].active_attr[i]);

	DEBUG_GL_ERR()

	glDrawElements(GL_TRIANGLES, g_num_faces[MESH_QUAD] * 3, GL_UNSIGNED_SHORT, 0);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_FBO].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_FBO].active_attr[i]);

	DEBUG_GL_ERR()

	return true;
}

} // namespace testbed
