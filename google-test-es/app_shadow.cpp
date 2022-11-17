#if defined(PLATFORM_GLX)

#include <GL/gl.h>
#include "gles_gl_mapping.hpp"

#else

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "pure_macro.hpp"

#if GL_OES_depth_texture == 0
#error Missing required extension GL_OES_depth_texture.
#endif

#if GL_OES_depth24 == 0
#error Missing required extension GL_OES_depth24.
#endif

#if GL_OES_packed_depth_stencil == 0
#error Missing required extension GL_OES_packed_depth_stencil.
#endif

#endif // PLATFORM_GLX

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

#include "rendVect.hpp"
#include "rendIndexedTrilist.hpp"
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

namespace sm
{

#define SETUP_VERTEX_ATTR_POINTERS_MASK	(			\
		SETUP_VERTEX_ATTR_POINTERS_MASK_vertex |	\
		SETUP_VERTEX_ATTR_POINTERS_MASK_normal)

#include "rendVertAttr_setupVertAttrPointers.hpp"
#undef SETUP_VERTEX_ATTR_POINTERS_MASK

struct Vertex
{
	GLfloat pos[3];
	GLfloat nrm[3];
};

} // namespace sm

namespace testbed
{

#define INSPECT_SHADOW			0
#define DEPTH_PRECISION_24		0

#define OPTION_IDENTIFIER_MAX	64

static const char arg_albedo[]		= "albedo_map";
static const char arg_shadow_res[]	= "shadow_res";
static const char arg_mesh_pn[]		= "mesh_pn";
static const char arg_mesh_pn2[]	= "mesh_pn2";
static const char arg_rot_axis[]	= "rotation_axis";
static const char arg_back_cast[]	= "back_cast";
static const char arg_invert_cull[]	= "invert_cull";
static const char arg_cam_extent[]	= "cam_extent";
static const char arg_cam_ortho[]	= "cam_ortho";
static const char arg_anim_step[]	= "anim_step";

static char g_albedo_filename[FILENAME_MAX + 1] = "graph_paper.raw";
static unsigned g_albedo_w = 512;
static unsigned g_albedo_h = 512;

static char g_mesh_filename[FILENAME_MAX + 1];
static enum {
	CUSTOM_MESH_NONE,
	CUSTOM_MESH_POSITION_NORMAL,
	CUSTOM_MESH_POSITION_NORMAL_TEXCOORD,
} g_custom_mesh;
static bool g_mesh_rotated;
static bool g_back_cast;

static GLenum g_face_front = GL_CCW;
static GLenum g_face_back  = GL_CW;

static struct {
	unsigned x : 1;
	unsigned y : 1;
	unsigned z : 1;
} g_axis = { 1, 1, 1 };

static rend::matx4 g_proj_cam;
static bool g_perspective_cam = true;
static float g_persp_extent = .5f;
static float g_ortho_extent = 2.f;
static float g_angle_step = 1.f / 40.f;

static const unsigned fbo_default_res = 2048;
static GLsizei g_fbo_res = fbo_default_res;

#if !defined(PLATFORM_GLX)

static EGLDisplay g_display = EGL_NO_DISPLAY;
static EGLContext g_context = EGL_NO_CONTEXT;

#endif

enum {
	TEX_SHADOW,
	TEX_SHADOW_DUMMY,
	TEX_ALBEDO,

	TEX_COUNT,
	TEX_FORCE_UINT = -1U
};

enum {
	PROG_MAIN_FG,
	PROG_MAIN_BG,
	PROG_SHADOW,
	PROG_INSPECTOR,

	PROG_COUNT,
	PROG_FORCE_UINT = -1U
};

enum {
	UNI_SAMPLER_SHADOW,
	UNI_SAMPLER_ALBEDO,

	UNI_LP_OBJ,
	UNI_VP_OBJ,

	UNI_MVP,
	UNI_MVP_LIT,

	UNI_COUNT,
	UNI_FORCE_UINT = -1U
};

enum {
	MESH_MAIN,
	MESH_QUAD,

	MESH_COUNT,
	MESH_FORCE_UINT = -1U
};

enum {
	VBO_MAIN_VTX,
	VBO_MAIN_IDX,
	VBO_QUAD_VTX,
	VBO_QUAD_IDX,

	VBO_COUNT,
	VBO_FORCE_UINT = -1U
};

static GLint g_uni[PROG_COUNT][UNI_COUNT];
static GLint g_vport[4];

#if defined(PLATFORM_GLX)

static GLuint g_vao[PROG_COUNT];

#endif

static GLuint g_fbo;
static GLuint g_rb;
static GLuint g_tex[TEX_COUNT];
static GLuint g_vbo[VBO_COUNT];
static GLuint g_shader_vert[PROG_COUNT];
static GLuint g_shader_frag[PROG_COUNT];
static GLuint g_shader_prog[PROG_COUNT];

static unsigned g_num_faces[MESH_COUNT];
static GLenum g_index_type;

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
	return true;
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
			unsigned rx = 0, ry = 0, rz = 0;
			float fx = 0.f;

			if (1 == sscanf(argv[i], "%" XQUOTE(OPTION_IDENTIFIER_MAX) "s %n", option, &opt_arg_start))
			{
				if (!strcmp(option, arg_albedo))
					if (3 == sscanf(argv[i] + opt_arg_start, "%" XQUOTE(FILENAME_MAX) "s %u %u",
								g_albedo_filename, &g_albedo_w, &g_albedo_h))
					{
						continue;
					}

				if (!strcmp(option, arg_shadow_res))
					if (1 == sscanf(argv[i] + opt_arg_start, "%u", &g_fbo_res) &&
						0 == (g_fbo_res & g_fbo_res - 1))
					{
						continue;
					}

				if (!strcmp(option, arg_mesh_pn))
					if (0 != sscanf(argv[i] + opt_arg_start, "%" XQUOTE(FILENAME_MAX) "s %u",
								g_mesh_filename, &rx))
					{
						g_mesh_rotated = 0 != rx;
						g_custom_mesh = CUSTOM_MESH_POSITION_NORMAL;
						continue;
					}

				if (!strcmp(option, arg_mesh_pn2))
					if (0 != sscanf(argv[i] + opt_arg_start, "%" XQUOTE(FILENAME_MAX) "s %u",
								g_mesh_filename, &rx))
					{
						g_mesh_rotated = 0 != rx;
						g_custom_mesh = CUSTOM_MESH_POSITION_NORMAL_TEXCOORD;
						continue;
					}

				if (!strcmp(option, arg_rot_axis))
					if (3 == sscanf(argv[i] + opt_arg_start, "%u %u %u",
								&rx, &ry, &rz))
					{
						g_axis.x = rx ? 1 : 0;
						g_axis.y = ry ? 1 : 0;
						g_axis.z = rz ? 1 : 0;
						continue;
					}

				if (!strcmp(option, arg_back_cast))
				{
					g_back_cast = true;
					continue;
				}

				if (!strcmp(option, arg_invert_cull))
				{
					g_face_front = GL_CW;
					g_face_back  = GL_CCW;
					continue;
				}

				if (!strcmp(option, arg_cam_extent))
					if (1 == sscanf(argv[i] + opt_arg_start, "%f", &fx))
					{
						g_persp_extent = fx;
						g_ortho_extent = fx;
						continue;
					}

				if (!strcmp(option, arg_cam_ortho))
				{
					g_perspective_cam = false;
					continue;
				}

				if (!strcmp(option, arg_anim_step))
					if (1 == sscanf(argv[i] + opt_arg_start, "%f", &g_angle_step))
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
			" <filename> <width> <height>\t: use specified raw file and dimensions as source of albedo map\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_mesh_pn <<
			" <filename> [<flag_rotated>]\t: use specified .mesh file as source of object (position and normal)\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_mesh_pn2 <<
			" <filename> [<flag_rotated>]\t: use specified .mesh file as source of object (position, normal, texcoord)\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_rot_axis <<
			" <flag_x> <flag_y> <flag_z>\t: set rotation animation around these axes; default is all\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_back_cast <<
			"\t\t\t\t\t: shadow cast by back-facing polygons in light space; default is front-facing\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_invert_cull <<
			"\t\t\t\t: invert polygon culling; default front-facing polygon winding is CCW\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_cam_extent <<
			" <extent>\t\t\t: use specified camera extent; default is .5 for persp, 2.0 for ortho\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_cam_ortho <<
			"\t\t\t\t\t: use orthographic camera; default is perspective\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_anim_step <<
			" <step>\t\t\t\t: use specified rotation step\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_shadow_res <<
			" <pot>\t\t\t\t: use specified shadow buffer resolution (POT); default is " << fbo_default_res << "\n" << std::endl;
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

	// tile texture mapping the following number of times:
	const float tile = 4.f;

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

		arr[ai].txc[0] = tile * (j + .5f) / (cols - 1);
		arr[ai].txc[1] = tile * .5f;

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

			arr[ai].txc[0] = tile * j / (cols - 1);
			arr[ai].txc[1] = tile * (rows - 1 - i) / (2 * rows - 2);

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

		arr[ai].txc[0] = tile * (j + .5f) / (cols - 1);
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

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_idx);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

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
	const float tile = 1.f;

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
bindVertexBuffersAndPointers< PROG_MAIN_FG >()
{
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_MAIN_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_MAIN_IDX]);

	DEBUG_GL_ERR()

	if (CUSTOM_MESH_POSITION_NORMAL == g_custom_mesh)
		return sm::setupVertexAttrPointers< sm::Vertex >(g_active_attr_semantics[PROG_MAIN_FG]);

	return sp::setupVertexAttrPointers< sp::Vertex >(g_active_attr_semantics[PROG_MAIN_FG]);
}


template <>
inline bool
bindVertexBuffersAndPointers< PROG_MAIN_BG >()
{
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_QUAD_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_QUAD_IDX]);

	DEBUG_GL_ERR()

	return qu::setupVertexAttrPointers< qu::Vertex >(g_active_attr_semantics[PROG_MAIN_BG]);
}


template <>
inline bool
bindVertexBuffersAndPointers< PROG_SHADOW >()
{
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_MAIN_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_MAIN_IDX]);

	DEBUG_GL_ERR()

	if (CUSTOM_MESH_POSITION_NORMAL == g_custom_mesh)
		return sm::setupVertexAttrPointers< sm::Vertex >(g_active_attr_semantics[PROG_SHADOW]);

	return sp::setupVertexAttrPointers< sp::Vertex >(g_active_attr_semantics[PROG_SHADOW]);
}


template <>
inline bool
bindVertexBuffersAndPointers< PROG_INSPECTOR >()
{
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_QUAD_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_QUAD_IDX]);

	DEBUG_GL_ERR()

	return qu::setupVertexAttrPointers< qu::Vertex >(g_active_attr_semantics[PROG_INSPECTOR]);
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

	glDeleteRenderbuffers(1, &g_rb);
	g_rb = 0;

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

	if (g_perspective_cam)
		g_proj_cam = rend::matx4().persp(
			-g_persp_extent, g_persp_extent,
			-g_persp_extent, g_persp_extent,
			.5f, 8.f);
	else
		g_proj_cam = rend::matx4().ortho(
			-g_ortho_extent, g_ortho_extent,
			-g_ortho_extent, g_ortho_extent,
			.5f, 8.f);

	scoped_ptr< deinit_resources_t, scoped_functor > on_error(deinit_resources);

	std::ostringstream patch_res[2];
	patch_res[0] << "const float shadow_res = " << fbo_default_res << ".0;";
	patch_res[1] << "const float shadow_res = " << g_fbo_res << ".0;";

	const std::string patch[] =
	{
		patch_res[0].str(),
		patch_res[1].str()
	};

	/////////////////////////////////////////////////////////////////

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glGetIntegerv(GL_VIEWPORT, g_vport);

	const GLclampf red	 = .5f;
	const GLclampf green = .5f;
	const GLclampf blue	 = .5f;
	const GLclampf alpha = 1.f;

	glClearColor(
		red, 
		green, 
		blue, 
		alpha);

	glClearDepthf(1.f);

	glPolygonOffset(2.5f, 4.f);

	/////////////////////////////////////////////////////////////////

	glGenTextures(sizeof(g_tex) / sizeof(g_tex[0]), g_tex);

	for (unsigned i = 0; i < sizeof(g_tex) / sizeof(g_tex[0]); ++i)
		assert(g_tex[i]);

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
	// init program for main target foreground
	/////////////////////////////////////////////////////////////////

	g_shader_vert[PROG_MAIN_FG] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_MAIN_FG]);

	if (!util::setupShader(g_shader_vert[PROG_MAIN_FG], "phong_shadow.glslv"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_frag[PROG_MAIN_FG] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_MAIN_FG]);

	if (!util::setupShaderWithPatch(g_shader_frag[PROG_MAIN_FG], "phong_shadow_pcf.glslf",
			sizeof(patch) / sizeof(patch[0]) / 2,
			patch))
	{
		std::cerr << __FUNCTION__ << " failed at setupShaderWithPatch" << std::endl;
		return false;
	}

	g_shader_prog[PROG_MAIN_FG] = glCreateProgram();
	assert(g_shader_prog[PROG_MAIN_FG]);

	if (!util::setupProgram(
			g_shader_prog[PROG_MAIN_FG],
			g_shader_vert[PROG_MAIN_FG],
			g_shader_frag[PROG_MAIN_FG]))
	{
		std::cerr << __FUNCTION__ << " failed at setupProgram" << std::endl;
		return false;
	}

	g_uni[PROG_MAIN_FG][UNI_MVP] =
		glGetUniformLocation(g_shader_prog[PROG_MAIN_FG], "mvp");
	g_uni[PROG_MAIN_FG][UNI_MVP_LIT] =
		glGetUniformLocation(g_shader_prog[PROG_MAIN_FG], "mvp_lit");
	g_uni[PROG_MAIN_FG][UNI_LP_OBJ] =
		glGetUniformLocation(g_shader_prog[PROG_MAIN_FG], "lp_obj");
	g_uni[PROG_MAIN_FG][UNI_VP_OBJ] =
		glGetUniformLocation(g_shader_prog[PROG_MAIN_FG], "vp_obj");

	g_uni[PROG_MAIN_FG][UNI_SAMPLER_SHADOW] =
		glGetUniformLocation(g_shader_prog[PROG_MAIN_FG], "shadow_map");
	g_uni[PROG_MAIN_FG][UNI_SAMPLER_ALBEDO] =
		glGetUniformLocation(g_shader_prog[PROG_MAIN_FG], "albedo_map");

	g_active_attr_semantics[PROG_MAIN_FG].registerVertexAttr(
		glGetAttribLocation(g_shader_prog[PROG_MAIN_FG], "at_Vertex"));
	g_active_attr_semantics[PROG_MAIN_FG].registerNormalAttr(
		glGetAttribLocation(g_shader_prog[PROG_MAIN_FG], "at_Normal"));
	g_active_attr_semantics[PROG_MAIN_FG].registerTCoordAttr(
		glGetAttribLocation(g_shader_prog[PROG_MAIN_FG], "at_MultiTexCoord0"));

	/////////////////////////////////////////////////////////////////
	// init program for main target background
	/////////////////////////////////////////////////////////////////

	g_shader_vert[PROG_MAIN_BG] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_MAIN_BG]);

	if (!util::setupShader(g_shader_vert[PROG_MAIN_BG], "mvp_texture_proj.glslv"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_frag[PROG_MAIN_BG] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_MAIN_BG]);

	if (!util::setupShader(g_shader_frag[PROG_MAIN_BG], "texture_proj.glslf"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_prog[PROG_MAIN_BG] = glCreateProgram();
	assert(g_shader_prog[PROG_MAIN_BG]);

	if (!util::setupProgram(
			g_shader_prog[PROG_MAIN_BG],
			g_shader_vert[PROG_MAIN_BG],
			g_shader_frag[PROG_MAIN_BG]))
	{
		std::cerr << __FUNCTION__ << " failed at setupProgram" << std::endl;
		return false;
	}

	g_uni[PROG_MAIN_BG][UNI_SAMPLER_ALBEDO] =
		glGetUniformLocation(g_shader_prog[PROG_MAIN_BG], "albedo_map");

	g_uni[PROG_MAIN_BG][UNI_MVP] =
		glGetUniformLocation(g_shader_prog[PROG_MAIN_BG], "mvp");
	g_uni[PROG_MAIN_BG][UNI_MVP_LIT] =
		glGetUniformLocation(g_shader_prog[PROG_MAIN_BG], "mvp_lit");

	g_active_attr_semantics[PROG_MAIN_BG].registerVertexAttr(
		glGetAttribLocation(g_shader_prog[PROG_MAIN_BG], "at_Vertex"));

	/////////////////////////////////////////////////////////////////
	// init program for shadow target
	/////////////////////////////////////////////////////////////////

	g_shader_vert[PROG_SHADOW] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_SHADOW]);

	if (!util::setupShader(g_shader_vert[PROG_SHADOW], "mvp.glslv"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_frag[PROG_SHADOW] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_SHADOW]);

	if (!util::setupShader(g_shader_frag[PROG_SHADOW], "depth.glslf"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_prog[PROG_SHADOW] = glCreateProgram();
	assert(g_shader_prog[PROG_SHADOW]);

	if (!util::setupProgram(
			g_shader_prog[PROG_SHADOW],
			g_shader_vert[PROG_SHADOW],
			g_shader_frag[PROG_SHADOW]))
	{
		std::cerr << __FUNCTION__ << " failed at setupProgram" << std::endl;
		return false;
	}

	g_uni[PROG_SHADOW][UNI_MVP] =
		glGetUniformLocation(g_shader_prog[PROG_SHADOW], "mvp");
	g_uni[PROG_SHADOW][UNI_LP_OBJ] =
		glGetUniformLocation(g_shader_prog[PROG_SHADOW], "lp_obj");

	g_active_attr_semantics[PROG_SHADOW].registerVertexAttr(
		glGetAttribLocation(g_shader_prog[PROG_SHADOW], "at_Vertex"));
	g_active_attr_semantics[PROG_SHADOW].registerNormalAttr(
		glGetAttribLocation(g_shader_prog[PROG_SHADOW], "at_Normal"));

	/////////////////////////////////////////////////////////////////
	// init program for shadow inspection
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

	if (!util::setupShader(g_shader_frag[PROG_INSPECTOR], "texture.glslf"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
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

	switch (g_custom_mesh)
	{
	case CUSTOM_MESH_POSITION_NORMAL:
		if (!util::fill_indexed_trilist_from_file_PN(
				g_mesh_filename,
				g_vbo[VBO_MAIN_VTX],
				g_vbo[VBO_MAIN_IDX],
				g_num_faces[MESH_MAIN],
				g_index_type,
				g_mesh_rotated))
		{
			g_custom_mesh = CUSTOM_MESH_NONE;
		}
		break;
	case CUSTOM_MESH_POSITION_NORMAL_TEXCOORD:
		if (!util::fill_indexed_trilist_from_file_PN2(
				g_mesh_filename,
				g_vbo[VBO_MAIN_VTX],
				g_vbo[VBO_MAIN_IDX],
				g_num_faces[MESH_MAIN],
				g_index_type,
				g_mesh_rotated))
		{
			g_custom_mesh = CUSTOM_MESH_NONE;
		}
		break;
	}

	if (CUSTOM_MESH_NONE == g_custom_mesh)
	{
		createIndexedPolarSphere(
			g_vbo[VBO_MAIN_VTX],
			g_vbo[VBO_MAIN_IDX],
			g_num_faces[MESH_MAIN]);

		g_index_type = GL_UNSIGNED_SHORT;
	}

	createIndexedGridQuad(
		g_vbo[VBO_QUAD_VTX],
		g_vbo[VBO_QUAD_IDX],
		g_num_faces[MESH_QUAD]);

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_SHADOW]);

	if (!bindVertexBuffersAndPointers< PROG_SHADOW >()
#if !defined(DEBUG)
		|| util::reportGLError()
#endif
		)
	{
		std::cerr << __FUNCTION__ <<
			"failed at bindVertexBuffersAndPointers for PROG_SHADOW" << std::endl;
		return false;
	}

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

	glBindVertexArray(g_vao[PROG_MAIN_FG]);

	if (!bindVertexBuffersAndPointers< PROG_MAIN_FG >()
#if !defined(DEBUG)
		|| util::reportGLError()
#endif
		)
	{
		std::cerr << __FUNCTION__ <<
			"failed at bindVertexBuffersAndPointers for PROG_MAIN_FG" << std::endl;
		return false;
	}

	glBindVertexArray(g_vao[PROG_MAIN_BG]);

	if (!bindVertexBuffersAndPointers< PROG_MAIN_BG >()
#if !defined(DEBUG)
		|| util::reportGLError()
#endif
		)
	{
		std::cerr << __FUNCTION__ <<
			"failed at bindVertexBuffersAndPointers for PROG_MAIN_BG" << std::endl;
		return false;
	}

	glBindVertexArray(0);

#endif

	/////////////////////////////////////////////////////////////////

	glGenFramebuffers(1, &g_fbo);
	assert(g_fbo);

	glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);

	glBindTexture(GL_TEXTURE_2D, g_tex[TEX_SHADOW]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#if DEPTH_PRECISION_24
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL_OES, g_fbo_res, g_fbo_res, 0,
		GL_DEPTH_STENCIL_OES, GL_UNSIGNED_INT_24_8_OES, 0);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, g_fbo_res, g_fbo_res, 0,
		GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 0);
#endif

	if (util::reportGLError())
	{
		std::cerr << __FUNCTION__ << " failed at shadow texture setup" << std::endl;
		return false;
	}

#if !defined(PLATFORM_GLX)

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenRenderbuffers(1, &g_rb);
	assert(g_rb);

	glBindRenderbuffer(GL_RENDERBUFFER, g_rb);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, g_fbo_res, g_fbo_res);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	if (util::reportGLError())
	{
		std::cerr << __FUNCTION__ << " failed at renderbuffer setup" << std::endl;
		return false;
	}

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, g_rb);

#else
	// under desktop GL use a full-blown texture rather than a renderbuffer; workaround for
	// a bug in catalyst 12.6 precluding the use of renderbuffers of GL_RGB5 format

	glBindTexture(GL_TEXTURE_2D, g_tex[TEX_SHADOW_DUMMY]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_fbo_res, g_fbo_res, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);

	if (util::reportGLError())
	{
		std::cerr << __FUNCTION__ << " failed at shadow dummy setup" << std::endl;
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_tex[TEX_SHADOW_DUMMY], 0);
#endif

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_tex[TEX_SHADOW], 0);
#if DEPTH_PRECISION_24
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, g_tex[TEX_SHADOW], 0);
#endif

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

	const float z_offset = -2.f;
	const float shadow_extent = 1.75f;

	static const rend::matx4 proj_lit = rend::matx4().ortho(
		-shadow_extent, shadow_extent,
		-shadow_extent, shadow_extent,
		0.f, 4.f);

	static const rend::vect3 x_lit(0.707107, 0.0, -0.707107);
	static const rend::vect3 y_lit(-.5, 0.707107, -.5);
	static const rend::vect3 z_lit(0.5, 0.707107, 0.5);

	static const rend::matx4 world_lit(
		x_lit[0], y_lit[0], z_lit[0], z_lit[0] * 2.f,
		x_lit[1], y_lit[1], z_lit[1], z_lit[1] * 2.f,
		x_lit[2], y_lit[2], z_lit[2], z_lit[2] * 2.f + z_offset,
		0.f,	  0.f,		0.f,	  1.f);

	static const rend::matx4 local_lit = rend::matx4().invert_unsafe(world_lit);
	static const rend::matx4 viewproj_lit = rend::matx4().mul(proj_lit, local_lit);

	static const rend::matx4 bias(
		.5f, 0.f, 0.f, .5f,
		0.f, .5f, 0.f, .5f,
		0.f, 0.f, .5f, .5f,
		0.f, 0.f, 0.f, 1.f);

	static const rend::matx4 biased_lit = rend::matx4().mul(bias, viewproj_lit);

	static GLfloat angle = 0.f;

	const rend::matx3& r0 = rend::matx3().rotate((g_axis.x ? angle : 0.f), 1.f, 0.f, 0.f);
	const rend::matx3& r1 = rend::matx3().rotate((g_axis.y ? angle : 0.f), 0.f, 1.f, 0.f);
	const rend::matx3& r2 = rend::matx3().rotate((g_axis.z ? angle : 0.f), 0.f, 0.f, 1.f);

	angle = fmodf(angle + g_angle_step, 2.f * M_PI);

	const rend::matx3& p0 = rend::matx3().mul(r1, r0);
	const rend::matx3& p1 = rend::matx3().mul(r2, p0);

	const rend::vect3 lp_obj = rend::vect3(
		p1[0][0] + p1[1][0] + p1[2][0],
		p1[0][1] + p1[1][1] + p1[2][1],
		p1[0][2] + p1[1][2] + p1[2][2]).normalise();

	const rend::matx4 mv_fg( // also object's world transform, as view is identity
		p1[0][0], p1[0][1], p1[0][2], 0.f,
		p1[1][0], p1[1][1], p1[1][2], 0.f,
		p1[2][0], p1[2][1], p1[2][2], z_offset,
		0.f,	  0.f,		0.f,	  1.f);

	static const rend::matx4 mv_bg( // also object's world transform, as view is identity
		x_lit[0] * shadow_extent, y_lit[0] * shadow_extent, z_lit[0] * shadow_extent, z_lit[0] * -2.f,
		x_lit[1] * shadow_extent, y_lit[1] * shadow_extent, z_lit[1] * shadow_extent, z_lit[1] * -2.f,
		x_lit[2] * shadow_extent, y_lit[2] * shadow_extent, z_lit[2] * shadow_extent, z_lit[2] * -2.f + z_offset,
		0.f,	  0.f,		0.f,	  1.f);

	const rend::matx4 mvp_lit_fg =
		rend::matx4().mul(viewproj_lit, mv_fg).transpose();

	const rend::matx4 biased_lit_fg =
		rend::matx4().mul(biased_lit, mv_fg).transpose();

	static const rend::matx4 mvp_lit_bg =
		rend::matx4().mul(viewproj_lit, mv_bg).transpose();

	static const rend::matx4 biased_lit_bg =
		rend::matx4().mul(biased_lit, mv_bg).transpose();

	const rend::matx4 mvp_fg =
		rend::matx4().mul(g_proj_cam, mv_fg).transpose();

	const rend::matx4 mvp_bg =
		rend::matx4().mul(g_proj_cam, mv_bg).transpose();

	/////////////////////////////////////////////////////////////////

	glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
	glViewport(0, 0, g_fbo_res, g_fbo_res);

	DEBUG_GL_ERR()

	if (g_back_cast)
		glFrontFace(g_face_back);
	else
		glFrontFace(g_face_front);

	glEnable(GL_POLYGON_OFFSET_FILL);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(g_shader_prog[PROG_SHADOW]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SHADOW][UNI_MVP])
	{
		glUniformMatrix4fv(g_uni[PROG_SHADOW][UNI_MVP], 1, GL_FALSE, mvp_lit_fg.decastF());
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SHADOW][UNI_LP_OBJ])
	{
		const GLfloat nonlocal_light[4] =
		{
			lp_obj[0],
			lp_obj[1],
			lp_obj[2],
			0.f
		};

		glUniform4fv(g_uni[PROG_SHADOW][UNI_LP_OBJ], 1, nonlocal_light);
	}

	DEBUG_GL_ERR()

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_SHADOW]);

	DEBUG_GL_ERR()

#else

	if (!bindVertexBuffersAndPointers< PROG_SHADOW >())
		return false;

#endif

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SHADOW].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SHADOW].active_attr[i]);

	DEBUG_GL_ERR()

	glDrawElements(GL_TRIANGLES, g_num_faces[MESH_MAIN] * 3, g_index_type, 0);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SHADOW].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_SHADOW].active_attr[i]);

	DEBUG_GL_ERR()

	/////////////////////////////////////////////////////////////////

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(g_vport[0], g_vport[1], g_vport[2], g_vport[3]);

	DEBUG_GL_ERR()

	if (g_back_cast)
		glFrontFace(g_face_front);

	glDisable(GL_POLYGON_OFFSET_FILL);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

#if INSPECT_SHADOW

	glFrontFace(GL_CCW);

	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(g_shader_prog[PROG_INSPECTOR]);

	DEBUG_GL_ERR()

	if (0 != g_tex[TEX_SHADOW] && -1 != g_uni[PROG_INSPECTOR][UNI_SAMPLER_ALBEDO])
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_SHADOW]);

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

	glDrawElements(GL_TRIANGLES, g_num_faces[MESH_QUAD] * 3, GL_UNSIGNED_SHORT, 0);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_INSPECTOR].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_INSPECTOR].active_attr[i]);

	DEBUG_GL_ERR()

	return true;

#endif // INSPECT_SHADOW

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(g_shader_prog[PROG_MAIN_FG]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_MAIN_FG][UNI_MVP])
	{
		glUniformMatrix4fv(g_uni[PROG_MAIN_FG][UNI_MVP], 1, GL_FALSE, mvp_fg.decastF());
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_MAIN_FG][UNI_MVP_LIT])
	{
		glUniformMatrix4fv(g_uni[PROG_MAIN_FG][UNI_MVP_LIT], 1, GL_FALSE, biased_lit_fg.decastF());
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_MAIN_FG][UNI_LP_OBJ])
	{
		const GLfloat nonlocal_light[4] =
		{
			lp_obj[0],
			lp_obj[1],
			lp_obj[2],
			0.f
		};

		glUniform4fv(g_uni[PROG_MAIN_FG][UNI_LP_OBJ], 1, nonlocal_light);
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_MAIN_FG][UNI_VP_OBJ])
	{
		const GLfloat nonlocal_viewer[4] =
		{
			p1[2][0],
			p1[2][1],
			p1[2][2],
			0.f
		};

		glUniform4fv(g_uni[PROG_MAIN_FG][UNI_VP_OBJ], 1, nonlocal_viewer);
	}

	DEBUG_GL_ERR()

	if (0 != g_tex[TEX_SHADOW] && -1 != g_uni[PROG_MAIN_FG][UNI_SAMPLER_SHADOW])
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_SHADOW]);

		glUniform1i(g_uni[PROG_MAIN_FG][UNI_SAMPLER_SHADOW], 0);
	}

	DEBUG_GL_ERR()

	if (0 != g_tex[TEX_ALBEDO] && -1 != g_uni[PROG_MAIN_FG][UNI_SAMPLER_ALBEDO])
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_ALBEDO]);

		glUniform1i(g_uni[PROG_MAIN_FG][UNI_SAMPLER_ALBEDO], 1);
	}

	DEBUG_GL_ERR()

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_MAIN_FG]);

	DEBUG_GL_ERR()

#else

	if (!bindVertexBuffersAndPointers< PROG_MAIN_FG >())
		return false;

#endif

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_MAIN_FG].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_MAIN_FG].active_attr[i]);

	DEBUG_GL_ERR()

	glDrawElements(GL_TRIANGLES, g_num_faces[MESH_MAIN] * 3, g_index_type, 0);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_MAIN_FG].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_MAIN_FG].active_attr[i]);

	DEBUG_GL_ERR()

	/////////////////////////////////////////////////////////////////

	glFrontFace(GL_CCW);

	glUseProgram(g_shader_prog[PROG_MAIN_BG]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_MAIN_BG][UNI_MVP])
	{
		glUniformMatrix4fv(g_uni[PROG_MAIN_BG][UNI_MVP], 1, GL_FALSE, mvp_bg.decastF());
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_MAIN_BG][UNI_MVP_LIT])
	{
		glUniformMatrix4fv(g_uni[PROG_MAIN_BG][UNI_MVP_LIT], 1, GL_FALSE, biased_lit_bg.decastF());
	}

	DEBUG_GL_ERR()

	if (0 != g_tex[TEX_SHADOW] && -1 != g_uni[PROG_MAIN_BG][UNI_SAMPLER_ALBEDO])
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_SHADOW]);

		glUniform1i(g_uni[PROG_MAIN_BG][UNI_SAMPLER_ALBEDO], 0);
	}

	DEBUG_GL_ERR()

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_MAIN_BG]);

	DEBUG_GL_ERR()

#else

	if (!bindVertexBuffersAndPointers< PROG_MAIN_BG >())
		return false;

#endif

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_MAIN_BG].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_MAIN_BG].active_attr[i]);

	DEBUG_GL_ERR()

	glDrawElements(GL_TRIANGLES, g_num_faces[MESH_QUAD] * 3, GL_UNSIGNED_SHORT, 0);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_MAIN_BG].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_MAIN_BG].active_attr[i]);

	DEBUG_GL_ERR()

	return true;
}

} // namespace testbed
