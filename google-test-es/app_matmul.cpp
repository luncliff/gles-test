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
#include <sstream>
#include <new>

#include "rendVect.hpp"
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

#define RESULT_PRECISION_32F	0

#define OPTION_IDENTIFIER_MAX	64

static const char arg_linear_map[] = "linear_map";
static const char arg_use_attrib[] = "use_attrib";

static bool g_linear_map;
static bool g_use_attrib;

#if !defined(PLATFORM_GLX)

static EGLDisplay g_display = EGL_NO_DISPLAY;
static EGLContext g_context = EGL_NO_CONTEXT;

#endif

enum {
	TEX_MAT_A,
	TEX_MAT_B,
	TEX_FBO,

	TEX_COUNT,
	TEX_FORCE_UINT = -1U
};

enum {
	PROG_MATMUL,

	PROG_COUNT,
	PROG_FORCE_UINT = -1U
};

enum {
	UNI_SAMPLER_MAT_A,
	UNI_SAMPLER_MAT_B,
	UNI_SAMPLER_FBO,

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

#if RESULT_PRECISION_32F == 1

static GLuint g_fbo;

#endif

static GLuint g_tex[TEX_COUNT];
static GLuint g_vbo[VBO_COUNT];
static GLuint g_shader_vert[PROG_COUNT];
static GLuint g_shader_frag[PROG_COUNT];
static GLuint g_shader_prog[PROG_COUNT];

static GLint g_vport[4];
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
				if (!strcmp(option, arg_linear_map))
				{
					g_linear_map = true;
					continue;
				}

				if (!strcmp(option, arg_use_attrib))
				{
					g_use_attrib = true;
					continue;
				}
			}
		}

		cli_err = true;
	}

	if (cli_err)
	{
		std::cerr << "app options (multiple args to an option must constitute a single string, eg. -foo \"a b c\"):\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_linear_map <<
			"\t: use matrix maps of linear arrangement\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_use_attrib <<
			"\t: use attribute-supplied work-item coordinates\n" << std::endl;
	}

	return !cli_err;
}


static bool
createIndexedQuad(
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	const unsigned map_w,
	const unsigned map_h,
	unsigned& num_faces)
{
	assert(vbo_arr && vbo_idx);

	const Vertex arr[] =
	{
		{
			{ 1.f, 1.f, 0.f },
			{ (map_w - .5f) / map_w, (map_h - .5f) / map_h }
		},
		{
			{ -1.f, 1.f, 0.f },
			{ .5f / map_w, (map_h - .5f) / map_h }
		},
		{
			{ -1.f, -1.f, 0.f },
			{ .5f / map_w, .5f / map_h }
		},
		{
			{ 1.f, -1.f, 0.f },
			{ (map_w - .5f) / map_w, .5f / map_h }
		}
	};

	const uint16_t idx[][3] = 
	{
		{ 0, 1, 2 },
		{ 2, 3, 0 }
	};

	num_faces = sizeof(idx) / sizeof(idx[0]);

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

#if RESULT_PRECISION_32F == 1

	glDeleteFramebuffers(1, &g_fbo);
	g_fbo = 0;

#endif

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

	glFrontFace(GL_CCW);

	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	glGetIntegerv(GL_VIEWPORT, g_vport);

	/////////////////////////////////////////////////////////////////

	const GLsizei map_w = g_vport[2];
	const GLsizei map_h = g_vport[3];

	if (g_linear_map)
	{
		if (map_w % 4)
		{
			std::cerr << "linear map mode requires viewport width "
				"to be a multiple of 4" << std::endl;
			return false;
		}
	}
	else
		if (map_w * map_h % 4)
		{
			std::cerr << "non-linear map mode requires viewport width "
				"and height to be multiples of 2" << std::endl;
			return false;
		}

	std::ostringstream patch_res;
	patch_res << "const vec2 tcoord_delta = vec2("
		"1.0 / " << map_w << ".0, "
		"1.0 / " << map_h << ".0);";

	const std::string patch[] =
	{
		"const vec2 tcoord_delta = vec2(1.0 / 512.0);",
		patch_res.str()
	};

	const unsigned mat_count = map_w * map_h / 4;

	scoped_ptr< rend::matx4, generic_delete_arr > identity_map(
		new (std::nothrow) rend::matx4[mat_count]);

	if (0 == identity_map())
	{
		std::cerr << __FUNCTION__ << " failed to allocate identity_map" << std::endl;
		return false;
	}

	const char* matmul_filename = 0;
	const rend::matx4 identity = rend::matx4().identity();

	if (g_linear_map)
	{
		if (g_use_attrib)
			matmul_filename = "matmul_1x4_varying.glslf";
		else
			matmul_filename = "matmul_1x4.glslf";

		for (unsigned i = 0; i < mat_count; ++i)
			identity_map()[i] = identity;
	}
	else
	{
		if (g_use_attrib)
			matmul_filename = "matmul_2x2_varying.glslf";
		else
			matmul_filename = "matmul_2x2.glslf";

		// swizzle identity_map to the format expected by the quad-arrangement matmul shader
		for (unsigned i = 0; i < map_h; ++i)
			for (unsigned j = 0; j < map_w; j += 2)
			{
				const unsigned mat_index = (i * map_w + j) / 4;
				const unsigned row_index = (i * map_w + j) % 4;

				if (i & 1)
				{
					// odd identity_map rows get identity matrix rows 2 & 3
					identity_map()[mat_index].setRow(row_index + 0, rend::vect4::cast(identity[2]));
					identity_map()[mat_index].setRow(row_index + 1, rend::vect4::cast(identity[3]));
				}
				else
				{
					// even identity_map rows get identity matrix rows 0 & 1
					identity_map()[mat_index].setRow(row_index + 0, rend::vect4::cast(identity[0]));
					identity_map()[mat_index].setRow(row_index + 1, rend::vect4::cast(identity[1]));
				}
			}
	}

	glGenTextures(sizeof(g_tex) / sizeof(g_tex[0]), g_tex);

	for (unsigned i = 0; i < sizeof(g_tex) / sizeof(g_tex[0]); ++i)
		assert(g_tex[i]);

	glBindTexture(GL_TEXTURE_2D, g_tex[TEX_MAT_A]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#if defined(PLATFORM_GLX)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, map_w, map_h, 0, GL_RGBA, GL_FLOAT, identity_map());
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, map_w, map_h, 0, GL_RGBA, GL_FLOAT, identity_map());
#endif

	if (util::reportGLError())
	{
		std::cerr << __FUNCTION__ << " failed creating TEX_MAT_A" << std::endl;
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, g_tex[TEX_MAT_B]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#if defined(PLATFORM_GLX)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, map_w, map_h, 0, GL_RGBA, GL_FLOAT, identity_map());
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, map_w, map_h, 0, GL_RGBA, GL_FLOAT, identity_map());
#endif

	if (util::reportGLError())
	{
		std::cerr << __FUNCTION__ << " failed creating TEX_MAT_B" << std::endl;
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	/////////////////////////////////////////////////////////////////

	for (unsigned i = 0; i < PROG_COUNT; ++i)
		for (unsigned j = 0; j < UNI_COUNT; ++j)
			g_uni[i][j] = -1;

	/////////////////////////////////////////////////////////////////

	g_shader_vert[PROG_MATMUL] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_MATMUL]);

	if (!util::setupShader(g_shader_vert[PROG_MATMUL], "texture.glslv"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_frag[PROG_MATMUL] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_MATMUL]);

	if (!util::setupShaderWithPatch(g_shader_frag[PROG_MATMUL], matmul_filename,
			sizeof(patch) / sizeof(patch[0]) / 2,
			patch))
	{
		std::cerr << __FUNCTION__ << " failed at setupShaderWithPatch" << std::endl;
		return false;
	}

	g_shader_prog[PROG_MATMUL] = glCreateProgram();
	assert(g_shader_prog[PROG_MATMUL]);

	if (!util::setupProgram(
			g_shader_prog[PROG_MATMUL],
			g_shader_vert[PROG_MATMUL],
			g_shader_frag[PROG_MATMUL]))
	{
		std::cerr << __FUNCTION__ << " failed at setupProgram" << std::endl;
		return false;
	}

	g_uni[PROG_MATMUL][UNI_SAMPLER_MAT_A] = glGetUniformLocation(g_shader_prog[PROG_MATMUL], "a_map");
	g_uni[PROG_MATMUL][UNI_SAMPLER_MAT_B] = glGetUniformLocation(g_shader_prog[PROG_MATMUL], "b_map");

	g_active_attr_semantics[PROG_MATMUL].registerVertexAttr(
		glGetAttribLocation(g_shader_prog[PROG_MATMUL], "at_Vertex"));
	g_active_attr_semantics[PROG_MATMUL].registerTCoordAttr(
		glGetAttribLocation(g_shader_prog[PROG_MATMUL], "at_MultiTexCoord0"));

	/////////////////////////////////////////////////////////////////

#if defined(PLATFORM_GLX)

	glGenVertexArrays(sizeof(g_vao) / sizeof(g_vao[0]), g_vao);

	for (unsigned i = 0; i < sizeof(g_vao) / sizeof(g_vao[0]); ++i)
		assert(g_vao[i]);

#endif

	glGenBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);

	for (unsigned i = 0; i < sizeof(g_vbo) / sizeof(g_vbo[0]); ++i)
		assert(g_vbo[i]);

	if (!createIndexedQuad(
			g_vbo[VBO_QUAD_VTX],
			g_vbo[VBO_QUAD_IDX],
			map_w,
			map_h,
			g_num_faces[MESH_QUAD]))
	{
		std::cerr << __FUNCTION__ << " failed at createIndexedQuad" << std::endl;
		return false;
	}

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_MATMUL]);

#endif

	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_QUAD_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_QUAD_IDX]);

	if (!setupVertexAttrPointers< Vertex >(g_active_attr_semantics[PROG_MATMUL])
#if !defined(DEBUG)
		|| util::reportGLError()
#endif
		)
	{
		std::cerr << __FUNCTION__ <<
			" failed at setupVertexAttrPointers" << std::endl;
		return false;
	}

	/////////////////////////////////////////////////////////////////

#if RESULT_PRECISION_32F == 1

	glGenFramebuffers(1, &g_fbo);
	assert(g_fbo);

	glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);

	glBindTexture(GL_TEXTURE_2D, g_tex[TEX_FBO]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#if defined(PLATFORM_GLX)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, map_w, map_h, 0, GL_RGBA, GL_FLOAT, 0);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, map_w, map_h, 0, GL_RGBA, GL_FLOAT, 0);
#endif

	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_tex[TEX_FBO], 0);

	const GLenum fbo_success = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (GL_FRAMEBUFFER_COMPLETE != fbo_success)
	{
		std::cerr << __FUNCTION__ << "failed creating fbo" << std::endl;
		return false;
	}

#endif

	on_error.reset();
	return true;
}


bool
hook::render_frame()
{
	if (!check_context(__FUNCTION__))
		return false;

	/////////////////////////////////////////////////////////////////

#if RESULT_PRECISION_32F == 1

	glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);

	DEBUG_GL_ERR()

#endif

	glUseProgram(g_shader_prog[PROG_MATMUL]);

	DEBUG_GL_ERR()

	if (g_tex[TEX_MAT_A] && -1 != g_uni[PROG_MATMUL][UNI_SAMPLER_MAT_A])
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_MAT_A]);

		glUniform1i(g_uni[PROG_MATMUL][UNI_SAMPLER_MAT_A], 0);
	}

	DEBUG_GL_ERR()

	if (g_tex[TEX_MAT_B] && -1 != g_uni[PROG_MATMUL][UNI_SAMPLER_MAT_B])
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_MAT_B]);

		glUniform1i(g_uni[PROG_MATMUL][UNI_SAMPLER_MAT_B], 1);
	}

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_MATMUL].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_MATMUL].active_attr[i]);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_num_drawcalls; ++i)
		glDrawElements(GL_TRIANGLES, g_num_faces[MESH_QUAD] * 3, GL_UNSIGNED_SHORT, 0);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_MATMUL].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_MATMUL].active_attr[i]);

	DEBUG_GL_ERR()

	/////////////////////////////////////////////////////////////////

#if RESULT_PRECISION_32F == 1

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	DEBUG_GL_ERR()

#endif

	return true;
}

} // namespace testbed
