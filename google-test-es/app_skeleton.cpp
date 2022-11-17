#if defined(PLATFORM_GLX)

#include <GL/gl.h>
#include "gles_gl_mapping.hpp"

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
#include "rendIndexedTrilist.hpp"
#include "rendSkeleton.hpp"
#include "utilTex.hpp"
#include "testbed.hpp"

#include "rendVertAttr.hpp"

namespace sk
{

#define SETUP_VERTEX_ATTR_POINTERS_MASK	(			\
		SETUP_VERTEX_ATTR_POINTERS_MASK_vertex |	\
		SETUP_VERTEX_ATTR_POINTERS_MASK_normal |	\
		SETUP_VERTEX_ATTR_POINTERS_MASK_blendw |	\
		SETUP_VERTEX_ATTR_POINTERS_MASK_tcoord)

#include "rendVertAttr_setupVertAttrPointers.hpp"
#undef SETUP_VERTEX_ATTR_POINTERS_MASK

struct Vertex
{
	GLfloat pos[3];
	GLfloat	bon[4];
	GLfloat nrm[3];
	GLfloat txc[2];
};

} // namespace sk

namespace st
{

#define SETUP_VERTEX_ATTR_POINTERS_MASK	(			\
		SETUP_VERTEX_ATTR_POINTERS_MASK_vertex)

#include "rendVertAttr_setupVertAttrPointers.hpp"
#undef SETUP_VERTEX_ATTR_POINTERS_MASK

struct Vertex
{
	GLfloat pos[3];
};

} // namespace st

namespace testbed
{

#define ANIM_SKELETON			1
#define DRAW_SKELETON			0

#define OPTION_IDENTIFIER_MAX	64

static const char arg_normal[]		= "normal_map";
static const char arg_albedo[]		= "albedo_map";
static const char arg_anim_step[]	= "anim_step";

static char g_normal_filename[FILENAME_MAX + 1] = "NMBalls.raw";
static unsigned g_normal_w = 256;
static unsigned g_normal_h = 256;

static char g_albedo_filename[FILENAME_MAX + 1] = "graph_paper.raw";
static unsigned g_albedo_w = 512;
static unsigned g_albedo_h = 512;

static char g_mesh_filename[FILENAME_MAX + 1] = "mesh/Ahmed_GEO.age";

static float g_anim_step = .125f * .125f * .25f;
static rend::matx4 g_matx_fit;

enum {
	BONE_CAPACITY	= 32
};

static unsigned g_bone_count;
static rend::Bone g_bone[BONE_CAPACITY + 1];
static rend::Bone* g_root_bone = g_bone + BONE_CAPACITY;
static rend::matx4 g_bone_mat[BONE_CAPACITY];
static std::vector< std::vector< rend::Track > > g_animations;

#if DRAW_SKELETON

static st::Vertex g_stick[BONE_CAPACITY][2];

#endif

#if !defined(PLATFORM_GLX)

static EGLDisplay g_display = EGL_NO_DISPLAY;
static EGLContext g_context = EGL_NO_CONTEXT;

#endif

enum {
	TEX_NORMAL,
	TEX_ALBEDO,

	TEX_COUNT,
	TEX_FORCE_UINT = -1U
};

enum {
	PROG_SKIN,
	PROG_SKEL,

	PROG_COUNT,
	PROG_FORCE_UINT = -1U
};

enum {
	UNI_SAMPLER_NORMAL,
	UNI_SAMPLER_ALBEDO,

	UNI_LP_OBJ,
	UNI_VP_OBJ,
	UNI_SOLID_COLOR,

	UNI_BONE,
	UNI_MVP,

	UNI_COUNT,
	UNI_FORCE_UINT = -1U
};

enum {
	MESH_SKIN,

	MESH_COUNT,
	MESH_FORCE_UINT = -1U
};

enum {
	VBO_SKIN_VTX,
	VBO_SKIN_IDX,
	VBO_SKEL_VTX,
	/* VBO_SKEL_IDX not required */

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
	const char* const* argv)
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

				if (!strcmp(option, arg_anim_step))
					if (1 == sscanf(argv[i] + opt_arg_start, "%f", &g_anim_step))
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
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_anim_step <<
			" <step>\t\t\t\t: use specified animation step; entire animation is 1.0\n" << std::endl;
	}

	return !cli_err;
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
bindVertexBuffersAndPointers< PROG_SKIN >()
{
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_SKIN_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_SKIN_IDX]);

	DEBUG_GL_ERR()

	return sk::setupVertexAttrPointers< sk::Vertex >(g_active_attr_semantics[PROG_SKIN]);
}

#if DRAW_SKELETON

template <>
inline bool
bindVertexBuffersAndPointers< PROG_SKEL >()
{
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_SKEL_VTX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	DEBUG_GL_ERR()

	glBufferData(GL_ARRAY_BUFFER, sizeof(g_stick[0]) * g_bone_count, g_stick,
		GL_STREAM_DRAW);

	DEBUG_GL_ERR()

	return st::setupVertexAttrPointers< st::Vertex >(g_active_attr_semantics[PROG_SKEL]);
}

#endif // DRAW_SKELETON

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

	const GLclampf red = .25f;
	const GLclampf green = .25f;
	const GLclampf blue = .25f;
	const GLclampf alpha = 1.f;

	glClearColor(
		red, 
		green, 
		blue, 
		alpha);

	glClearDepthf(1.f);

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

	g_shader_vert[PROG_SKIN] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_SKIN]);

	if (!util::setupShader(g_shader_vert[PROG_SKIN], "phong_skinning_matsum.glslv"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_frag[PROG_SKIN] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_SKIN]);

	if (!util::setupShader(g_shader_frag[PROG_SKIN], "phong.glslf"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_prog[PROG_SKIN] = glCreateProgram();
	assert(g_shader_prog[PROG_SKIN]);

	if (!util::setupProgram(
			g_shader_prog[PROG_SKIN],
			g_shader_vert[PROG_SKIN],
			g_shader_frag[PROG_SKIN]))
	{
		std::cerr << __FUNCTION__ << " failed at setupProgram" << std::endl;
		return false;
	}

	g_uni[PROG_SKIN][UNI_MVP]		= glGetUniformLocation(g_shader_prog[PROG_SKIN], "mvp");
	g_uni[PROG_SKIN][UNI_BONE]		= glGetUniformLocation(g_shader_prog[PROG_SKIN], "bone");
	g_uni[PROG_SKIN][UNI_LP_OBJ]	= glGetUniformLocation(g_shader_prog[PROG_SKIN], "lp_obj");
	g_uni[PROG_SKIN][UNI_VP_OBJ]	= glGetUniformLocation(g_shader_prog[PROG_SKIN], "vp_obj");

	g_uni[PROG_SKIN][UNI_SAMPLER_NORMAL] = glGetUniformLocation(g_shader_prog[PROG_SKIN], "normal_map");
	g_uni[PROG_SKIN][UNI_SAMPLER_ALBEDO] = glGetUniformLocation(g_shader_prog[PROG_SKIN], "albedo_map");

	g_active_attr_semantics[PROG_SKIN].registerVertexAttr(
		glGetAttribLocation(g_shader_prog[PROG_SKIN], "at_Vertex"));
	g_active_attr_semantics[PROG_SKIN].registerNormalAttr(
		glGetAttribLocation(g_shader_prog[PROG_SKIN], "at_Normal"));
	g_active_attr_semantics[PROG_SKIN].registerBlendWAttr(
		glGetAttribLocation(g_shader_prog[PROG_SKIN], "at_Weight"));
	g_active_attr_semantics[PROG_SKIN].registerTCoordAttr(
		glGetAttribLocation(g_shader_prog[PROG_SKIN], "at_MultiTexCoord0"));

	/////////////////////////////////////////////////////////////////

	g_shader_vert[PROG_SKEL] = glCreateShader(GL_VERTEX_SHADER);
	assert(g_shader_vert[PROG_SKEL]);

	if (!util::setupShader(g_shader_vert[PROG_SKEL], "mvp.glslv"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_frag[PROG_SKEL] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_SKEL]);

	if (!util::setupShader(g_shader_frag[PROG_SKEL], "basic.glslf"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_prog[PROG_SKEL] = glCreateProgram();
	assert(g_shader_prog[PROG_SKEL]);

	if (!util::setupProgram(
			g_shader_prog[PROG_SKEL],
			g_shader_vert[PROG_SKEL],
			g_shader_frag[PROG_SKEL]))
	{
		std::cerr << __FUNCTION__ << " failed at setupProgram" << std::endl;
		return false;
	}

	g_uni[PROG_SKEL][UNI_MVP]			= glGetUniformLocation(g_shader_prog[PROG_SKEL], "mvp");
	g_uni[PROG_SKEL][UNI_SOLID_COLOR]	= glGetUniformLocation(g_shader_prog[PROG_SKEL], "solid_color");

	g_active_attr_semantics[PROG_SKEL].registerVertexAttr(
		glGetAttribLocation(g_shader_prog[PROG_SKEL], "at_Vertex"));

	/////////////////////////////////////////////////////////////////
	const char* const skeleton_name = "mesh/Ahmed_GEO.skeleton";
	g_bone_count = BONE_CAPACITY;

	if (!rend::loadSkeletonAnimationAge(skeleton_name, &g_bone_count, g_bone_mat, g_bone, g_animations))
	{
		std::cerr << __FUNCTION__ << " failed to load skeleton file " << skeleton_name << std::endl;
		return false;
	}

	/////////////////////////////////////////////////////////////////

#if defined(PLATFORM_GLX)

	glGenVertexArrays(sizeof(g_vao) / sizeof(g_vao[0]), g_vao);

	for (unsigned i = 0; i < sizeof(g_vao) / sizeof(g_vao[0]); ++i)
		assert(g_vao[i]);

#endif

	glGenBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);

	for (unsigned i = 0; i < sizeof(g_vbo) / sizeof(g_vbo[0]); ++i)
		assert(g_vbo[i]);

	float bbox_min[3];
	float bbox_max[3];

	const GLfloat (sk::Vertex::* const soff_pos)[3] = &sk::Vertex::pos;
	const GLfloat (sk::Vertex::* const soff_bon)[4] = &sk::Vertex::bon;
	const GLfloat (sk::Vertex::* const soff_nrm)[3] = &sk::Vertex::nrm;
	const GLfloat (sk::Vertex::* const soff_txc)[2] = &sk::Vertex::txc;

	const uintptr_t semantics_offset[4] =
	{
		uintptr_t(*reinterpret_cast< const void* const* >(&soff_pos)),
		uintptr_t(*reinterpret_cast< const void* const* >(&soff_bon)),
		uintptr_t(*reinterpret_cast< const void* const* >(&soff_nrm)),
		uintptr_t(*reinterpret_cast< const void* const* >(&soff_txc))
	};

	if (!util::fill_indexed_trilist_from_file_AGE(
			g_mesh_filename,
			g_vbo[VBO_SKIN_VTX],
			g_vbo[VBO_SKIN_IDX],
			semantics_offset,
			g_num_faces[MESH_SKIN],
			g_index_type,
			bbox_min,
			bbox_max))
	{
		std::cerr << __FUNCTION__ << " failed at fill_indexed_trilist_from_file_AGE" << std::endl;
		return false;
	}

	const float centre[3] =
	{
		(bbox_min[0] + bbox_max[0]) * .5f,
		(bbox_min[1] + bbox_max[1]) * .5f,
		(bbox_min[2] + bbox_max[2]) * .5f
	};
	const float extent[3] =
	{
		(bbox_max[0] - bbox_min[0]) * .5f,
		(bbox_max[1] - bbox_min[1]) * .5f,
		(bbox_max[2] - bbox_min[2]) * .5f
	};
	const float rcp_extent = 1.f / (extent[0] > extent[1]
		? (extent[0] > extent[2] ? extent[0] : extent[2])
		: (extent[1] > extent[2] ? extent[1] : extent[2]));

	g_matx_fit = rend::matx4(
		rcp_extent,	0.f,		0.f,		-centre[0] * rcp_extent,
		0.f,		rcp_extent,	0.f,		-centre[1] * rcp_extent,
		0.f,		0.f,		rcp_extent,	-centre[2] * rcp_extent,
		0.f,		0.f,		0.f,		1.f);

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_SKIN]);

	if (!bindVertexBuffersAndPointers< PROG_SKIN >()
#if !defined(DEBUG)
		|| util::reportGLError()
#endif
		)
	{
		std::cerr << __FUNCTION__ <<
			"failed at bindVertexBuffersAndPointers for PROG_SKIN" << std::endl;
		return false;
	}

	glBindVertexArray(0);

#endif

	on_error.reset();
	return true;
}


bool
hook::render_frame()
{
	if (!check_context(__FUNCTION__))
		return false;

#if ANIM_SKELETON
	static std::vector< std::vector< rend::Track > >::const_iterator at = g_animations.begin();
	static float anim = 0.f;

	rend::animateSkeleton(g_bone_count, g_bone_mat, g_bone, *at, anim, g_root_bone);

	anim += g_anim_step;

	if (1.f < anim)
	{
		anim -= floorf(anim);
		rend::resetSkeletonAnimProgress(*at);

		if (g_animations.end() == ++at)
			at = g_animations.begin();
	}
#endif

#if DRAW_SKELETON
	for (unsigned i = 1; i < g_bone_count; ++i)
	{
		const unsigned j = g_bone[i].parent_idx;
		g_bone[j].to_model.getColS(3, rend::vect3::cast(g_stick[i][0].pos));
		g_bone[i].to_model.getColS(3, rend::vect3::cast(g_stick[i][1].pos));
	}
#endif

	const rend::matx4 mv = rend::matx4().translate(0.f, 0.f, -2.125f).mur(g_matx_fit).mur(g_root_bone->to_model);

#if 1
	static const rend::matx4 proj = rend::matx4().persp(-.5f, .5f, -.5f, .5f, 1.f, 4.f);
#else
	static const rend::matx4 proj = rend::matx4().ortho(-1.f, 1.f, -1.f, 1.f, 1.f, 4.f);
#endif

	const rend::matx4 mvp = rend::matx4().mul(proj, mv).transpose(); // ES cannot transpose on the fly

	const rend::vect3 lp_obj = rend::vect3(
		mv[0][0] + mv[2][0],
		mv[0][1] + mv[2][1],
		mv[0][2] + mv[2][2]).normalise();

	/////////////////////////////////////////////////////////////////

	glEnable(GL_DEPTH_TEST);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(g_shader_prog[PROG_SKIN]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKIN][UNI_MVP])
	{
		glUniformMatrix4fv(g_uni[PROG_SKIN][UNI_MVP],
			1, GL_FALSE, reinterpret_cast< const GLfloat* >((const float(&)[4][4]) mvp));
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKIN][UNI_BONE])
	{
		glUniformMatrix4fv(g_uni[PROG_SKIN][UNI_BONE],
			g_bone_count, GL_FALSE, reinterpret_cast< GLfloat* >(g_bone_mat));
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKIN][UNI_LP_OBJ])
	{
		const GLfloat nonlocal_light[4] =
		{
			lp_obj[0],
			lp_obj[1],
			lp_obj[2],
			0.f
		};

		glUniform4fv(g_uni[PROG_SKIN][UNI_LP_OBJ], 1, nonlocal_light);
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKIN][UNI_VP_OBJ])
	{
		const GLfloat nonlocal_viewer[4] =
		{
			mv[2][0],
			mv[2][1],
			mv[2][2],
			0.f
		};

		glUniform4fv(g_uni[PROG_SKIN][UNI_VP_OBJ], 1, nonlocal_viewer);
	}

	DEBUG_GL_ERR()

	if (0 != g_tex[TEX_NORMAL] && -1 != g_uni[PROG_SKIN][UNI_SAMPLER_NORMAL])
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_NORMAL]);

		glUniform1i(g_uni[PROG_SKIN][UNI_SAMPLER_NORMAL], 0);
	}

	DEBUG_GL_ERR()

	if (0 != g_tex[TEX_ALBEDO] && -1 != g_uni[PROG_SKIN][UNI_SAMPLER_ALBEDO])
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_ALBEDO]);

		glUniform1i(g_uni[PROG_SKIN][UNI_SAMPLER_ALBEDO], 1);
	}

	DEBUG_GL_ERR()

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_SKIN]);

	DEBUG_GL_ERR()

#else

	if (!bindVertexBuffersAndPointers< PROG_SKIN >())
		return false;

#endif

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKIN].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SKIN].active_attr[i]);

	DEBUG_GL_ERR()

	glDrawElements(GL_TRIANGLES, g_num_faces[MESH_SKIN] * 3, g_index_type, 0);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKIN].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_SKIN].active_attr[i]);

	DEBUG_GL_ERR()

	/////////////////////////////////////////////////////////////////
#if DRAW_SKELETON

	glDisable(GL_DEPTH_TEST);

	glUseProgram(g_shader_prog[PROG_SKEL]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKEL][UNI_MVP])
	{
		glUniformMatrix4fv(g_uni[PROG_SKEL][UNI_MVP],
			1, GL_FALSE, reinterpret_cast< const GLfloat* >((const float(&)[4][4]) mvp));
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKEL][UNI_SOLID_COLOR])
	{
		const GLfloat solid_color[4] = { 0.f, 1.f, 0.f, 1.f };

		glUniform4fv(g_uni[PROG_SKEL][UNI_SOLID_COLOR], 1, solid_color);
	}

	DEBUG_GL_ERR()

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_SKEL]);

	DEBUG_GL_ERR()

#endif

	if (!bindVertexBuffersAndPointers< PROG_SKEL >())
		return false;

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKEL].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SKEL].active_attr[i]);

	DEBUG_GL_ERR()

	glDrawArrays(GL_LINES, 0, g_bone_count * 2);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKEL].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_SKEL].active_attr[i]);

	DEBUG_GL_ERR()

#endif // DRAW_SKELETON

	return true;
}

} // namespace testbed
