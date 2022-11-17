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

#include "rendSkeleton.hpp"
#include "utilTex.hpp"
#include "testbed.hpp"

#include "rendVertAttr.hpp"

namespace
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
	GLfloat nrm[3];
	GLfloat	bon[4];
	GLfloat txc[2];
};

} // namespace

namespace testbed
{

#define OPTION_IDENTIFIER_MAX	64

static const char arg_normal[] = "normal_map";
static const char arg_albedo[] = "albedo_map";
static const char arg_alt_anim[] = "alt_anim";
static const char arg_anim_step[] = "anim_step";

static char g_normal_filename[FILENAME_MAX + 1] = "rockwall_NH.raw";
static unsigned g_normal_w = 64;
static unsigned g_normal_h = 64;

static char g_albedo_filename[FILENAME_MAX + 1] = "rockwall.raw";
static unsigned g_albedo_w = 256;
static unsigned g_albedo_h = 256;

static const unsigned bone_count = 13;

static rend::Bone g_bone[bone_count];
static rend::matx4 g_bone_mat[bone_count];
static std::vector< rend::Track > g_skeletal_animation;
static bool g_alt_anim;
static float g_anim_step = .125f * .125f * .125f;

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

	PROG_COUNT,
	PROG_FORCE_UINT = -1U
};

enum {
	UNI_SAMPLER_NORMAL,
	UNI_SAMPLER_ALBEDO,

	UNI_LP_OBJ,
	UNI_VP_OBJ,
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

				if (!strcmp(option, arg_alt_anim))
				{
					g_alt_anim = true;
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
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_alt_anim <<
			"\t\t\t\t\t: use alternative skeleton animation\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_anim_step <<
			" <step>\t\t\t\t: use specified animation step; entire animation is 1.0\n" << std::endl;
	}

	return !cli_err;
}


static void
setupSkeletonAnimA()
{
	const rend::quat identq(0.f, 0.f, 0.f, 1.f);

	rend::Track& track = *g_skeletal_animation.insert(g_skeletal_animation.end(), rend::Track());
	track.bone_idx = 0;

	rend::Track::BoneOrientationKey& ori0 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

	ori0.time = 0.f;
	ori0.value = identq;

	rend::Track::BoneOrientationKey& ori1 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

	ori1.time = .25f;
	ori1.value = rend::quat(M_PI * .25f, rend::vect3(1.f, 1.f, 1.f).normalise());

	rend::Track::BoneOrientationKey& ori2 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

	ori2.time = .75f;
	ori2.value = rend::quat(M_PI * -.25f, rend::vect3(1.f, 1.f, 1.f).normalise());

	rend::Track::BoneOrientationKey& ori3 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

	ori3.time = 1.f;
	ori3.value = identq;

	track.position_last_key_idx = 0;
	track.orientation_last_key_idx = 0;
	track.scale_last_key_idx = 0;

	static const uint8_t anim_bone[] =
	{
		4, 5, 10, 11
	};

	for (unsigned i = 0; i < sizeof(anim_bone) / sizeof(anim_bone[0]); ++i)
	{
		rend::Track& track = *g_skeletal_animation.insert(g_skeletal_animation.end(), rend::Track());
		track.bone_idx = anim_bone[i];

		rend::Track::BoneOrientationKey& ori0 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

		ori0.time = 0.f;
		ori0.value = identq;

		rend::Track::BoneOrientationKey& ori1 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

		ori1.time = .25f;
		ori1.value = rend::quat(M_PI * .375f, rend::vect3(0.f, 1.f, 0.f));

		rend::Track::BoneOrientationKey& ori2 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

		ori2.time = .5f;
		ori2.value = identq;

		rend::Track::BoneOrientationKey& ori3 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

		ori3.time = .75f;
		ori3.value = rend::quat(M_PI * -.375f, rend::vect3(0.f, 1.f, 0.f));

		rend::Track::BoneOrientationKey& ori4 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

		ori4.time = 1.f;
		ori4.value = identq;

		track.position_last_key_idx = 0;
		track.orientation_last_key_idx = 0;
		track.scale_last_key_idx = 0;
	}
}


static void
setupSkeletonAnimB()
{
	const rend::quat identq(0.f, 0.f, 0.f, 1.f);

	rend::Track& track = *g_skeletal_animation.insert(g_skeletal_animation.end(), rend::Track());
	track.bone_idx = 0;

	rend::Track::BoneOrientationKey& ori0 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

	ori0.time = 0.f;
	ori0.value = identq;

	rend::Track::BoneOrientationKey& ori1 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

	ori1.time = .25f;
	ori1.value = rend::quat(M_PI * .5f, rend::vect3(1.f, 1.f, 0.f).normalise());

	rend::Track::BoneOrientationKey& ori2 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

	ori2.time = .5f;
	ori2.value = rend::quat(M_PI, rend::vect3(1.f, 1.f, 0.f).normalise());

	rend::Track::BoneOrientationKey& ori3 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

	ori3.time = .75f;
	ori3.value = rend::quat(M_PI * 1.5f, rend::vect3(1.f, 1.f, 0.f).normalise());

	rend::Track::BoneOrientationKey& ori4 =
		*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

	ori4.time = 1.f;
	ori4.value = rend::quat(M_PI * 2.f, rend::vect3(1.f, 1.f, 0.f).normalise());

	track.position_last_key_idx = 0;
	track.orientation_last_key_idx = 0;
	track.scale_last_key_idx = 0;

	static const uint8_t anim_bone[] =
	{
		1, 2, 4, 5, 7, 8, 10, 11
	};

	for (unsigned i = 0; i < sizeof(anim_bone) / sizeof(anim_bone[0]); ++i)
	{
		const rend::vect3& axis = i % 4 & 2
			? rend::vect3(0.f, 1.f, 0.f)
			: rend::vect3(1.f, 0.f, 0.f);

		const float angle = 1 == i / 2 || 2 == i / 2
			? -.375f
			: .375f;

		rend::Track& track = *g_skeletal_animation.insert(g_skeletal_animation.end(), rend::Track());
		track.bone_idx = anim_bone[i];

		rend::Track::BoneOrientationKey& ori0 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

		ori0.time = 0.f;
		ori0.value = identq;

		rend::Track::BoneOrientationKey& ori1 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

		ori1.time = .25f;
		ori1.value = rend::quat(M_PI * angle, axis);

		rend::Track::BoneOrientationKey& ori2 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

		ori2.time = .5f;
		ori2.value = identq;

		rend::Track::BoneOrientationKey& ori3 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

		ori3.time = .75f;
		ori3.value = rend::quat(-M_PI * angle, axis);

		rend::Track::BoneOrientationKey& ori4 =
			*track.orientation_key.insert(track.orientation_key.end(), rend::Track::BoneOrientationKey());

		ori4.time = 1.f;
		ori4.value = identq;

		track.position_last_key_idx = 0;
		track.orientation_last_key_idx = 0;
		track.scale_last_key_idx = 0;
	}
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

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

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

	if (!util::setupShader(g_shader_vert[PROG_SKIN], "phong_skinning_bump_tang_workaround.glslv"))
	{
		std::cerr << __FUNCTION__ << " failed at setupShader" << std::endl;
		return false;
	}

	g_shader_frag[PROG_SKIN] = glCreateShader(GL_FRAGMENT_SHADER);
	assert(g_shader_frag[PROG_SKIN]);

	if (!util::setupShader(g_shader_frag[PROG_SKIN], "phong_bump_tang.glslf"))
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

	g_uni[PROG_SKIN][UNI_MVP]	 = glGetUniformLocation(g_shader_prog[PROG_SKIN], "mvp");
	g_uni[PROG_SKIN][UNI_BONE]	 = glGetUniformLocation(g_shader_prog[PROG_SKIN], "bone");
	g_uni[PROG_SKIN][UNI_LP_OBJ] = glGetUniformLocation(g_shader_prog[PROG_SKIN], "lp_obj");
	g_uni[PROG_SKIN][UNI_VP_OBJ] = glGetUniformLocation(g_shader_prog[PROG_SKIN], "vp_obj");

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

	const float bolen = .25f;

	const rend::quat identq(0.f, 0.f, 0.f, 1.f);
	const rend::vect3 idents(1.f, 1.f, 1.f);

	g_bone[0].position = rend::vect3(0.f, 0.f, 0.f);
	g_bone[0].orientation = identq;
	g_bone[0].scale = idents;
	g_bone[0].parent_idx = 255;

	g_bone[1].position = rend::vect3(0.f, bolen, 0.f);
	g_bone[1].orientation = identq;
	g_bone[1].scale = idents;
	g_bone[1].parent_idx = 0;

	g_bone[2].position = rend::vect3(0.f, bolen, 0.f);
	g_bone[2].orientation = identq;
	g_bone[2].scale = idents;
	g_bone[2].parent_idx = 1;

	g_bone[3].position = rend::vect3(0.f, bolen, 0.f);
	g_bone[3].orientation = identq;
	g_bone[3].scale = idents;
	g_bone[3].parent_idx = 2;

	g_bone[4].position = rend::vect3(-bolen, 0.f, 0.f);
	g_bone[4].orientation = identq;
	g_bone[4].scale = idents;
	g_bone[4].parent_idx = 0;

	g_bone[5].position = rend::vect3(-bolen, 0.f, 0.f);
	g_bone[5].orientation = identq;
	g_bone[5].scale = idents;
	g_bone[5].parent_idx = 4;

	g_bone[6].position = rend::vect3(-bolen, 0.f, 0.f);
	g_bone[6].orientation = identq;
	g_bone[6].scale = idents;
	g_bone[6].parent_idx = 5;

	g_bone[7].position = rend::vect3(0.f, -bolen, 0.f);
	g_bone[7].orientation = identq;
	g_bone[7].scale = idents;
	g_bone[7].parent_idx = 0;

	g_bone[8].position = rend::vect3(0.f, -bolen, 0.f);
	g_bone[8].orientation = identq;
	g_bone[8].scale = idents;
	g_bone[8].parent_idx = 7;

	g_bone[9].position = rend::vect3(0.f, -bolen, 0.f);
	g_bone[9].orientation = identq;
	g_bone[9].scale = idents;
	g_bone[9].parent_idx = 8;

	g_bone[10].position = rend::vect3(bolen, 0.f, 0.f);
	g_bone[10].orientation = identq;
	g_bone[10].scale = idents;
	g_bone[10].parent_idx = 0;

	g_bone[11].position = rend::vect3(bolen, 0.f, 0.f);
	g_bone[11].orientation = identq;
	g_bone[11].scale = idents;
	g_bone[11].parent_idx = 10;

	g_bone[12].position = rend::vect3(bolen, 0.f, 0.f);
	g_bone[12].orientation = identq;
	g_bone[12].scale = idents;
	g_bone[12].parent_idx = 11;

	for (unsigned i = 0; i < bone_count; ++i)
		rend::initBoneMatx(bone_count, g_bone_mat, g_bone, i);

	if (g_alt_anim)
		setupSkeletonAnimB();
	else
		setupSkeletonAnimA();

	/////////////////////////////////////////////////////////////////

	static const Vertex arr[] =
	{
		{ { 0.f, 0.f, 0.f },			{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 0.f },	{ .5f, .5f } },

		// north sleeve
		{ { bolen, 1.f * bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 0.f },	{ .5f + .5f / 3.f, .5f + .5f / 3.f } },
		{ {-bolen, 1.f * bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 0.f },	{ .5f - .5f / 3.f, .5f + .5f / 3.f } },
		{ { bolen, 2.f * bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 2.f },	{ .5f + .5f / 3.f, .5f + 1.f / 3.f } },
		{ {-bolen, 2.f * bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 2.f },	{ .5f - .5f / 3.f, .5f + 1.f / 3.f } },
		{ { bolen, 3.f * bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 3.f },	{ .5f + .5f / 3.f, 1.f } },
		{ {-bolen, 3.f * bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 3.f },	{ .5f - .5f / 3.f, 1.f } },

		// south sleeve
		{ { bolen, -1.f * bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 0.f },	{ .5f + .5f / 3.f, .5f - .5f / 3.f } },
		{ {-bolen, -1.f * bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 0.f },	{ .5f - .5f / 3.f, .5f - .5f / 3.f } },
		{ { bolen, -2.f * bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 8.f },	{ .5f + .5f / 3.f, .5f - 1.f / 3.f } },
		{ {-bolen, -2.f * bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 8.f },	{ .5f - .5f / 3.f, .5f - 1.f / 3.f } },
		{ { bolen, -3.f * bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 9.f },	{ .5f + .5f / 3.f, 0.f } },
		{ {-bolen, -3.f * bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 9.f },	{ .5f - .5f / 3.f, 0.f } },

		// east sleeve
		{ { 2.f * bolen,  bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 11.f }, { .5f + 1.f / 3.f, .5f + .5f / 3.f } },
		{ { 2.f * bolen, -bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 11.f }, { .5f + 1.f / 3.f, .5f - .5f / 3.f } },
		{ { 3.f * bolen,  bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 12.f }, { 1.f,             .5f + .5f / 3.f } },
		{ { 3.f * bolen, -bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 12.f }, { 1.f,             .5f - .5f / 3.f } },

		// west sleeve
		{ {-2.f * bolen,  bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 5.f },	{ .5f - 1.f / 3.f, .5f + .5f / 3.f } },
		{ {-2.f * bolen, -bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 5.f },	{ .5f - 1.f / 3.f, .5f - .5f / 3.f } },
		{ {-3.f * bolen,  bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 6.f },	{ 0.f,             .5f + .5f / 3.f } },
		{ {-3.f * bolen, -bolen, 0.f },	{ 0.f, 0.f, 1.f },	{ 1.f, 0.f, 0.f, 6.f },	{ 0.f,             .5f - .5f / 3.f } }
	};

	static const uint16_t idx[][3] =
	{
		{  0,  1,  2 },
		{  1,  3,  2 },
		{  2,  3,  4 },
		{  3,  5,  4 },
		{  4,  5,  6 },

		{  0,  8,  7 },
		{  7,  8,  9 },
		{  9,  8, 10 },
		{  9, 10, 11 },
		{ 11, 10, 12 },

		{  0,  7,  1 },
		{  7, 14,  1 },
		{  1, 14, 13 },
		{ 14, 16, 13 },
		{ 13, 16, 15 },

		{  0,  2,  8 },
		{  8,  2, 18 },
		{ 18,  2, 17 },
		{ 18, 17, 20 },
		{ 20, 17, 19 }
	};

	g_num_faces[MESH_SKIN] = sizeof(idx) / sizeof(idx[0]);

	const unsigned num_verts = sizeof(arr) / sizeof(arr[0]);
	const unsigned num_indes = sizeof(idx) / sizeof(idx[0][0]);

	std::cout << "number of vertices: " << num_verts <<
		"\nnumber of indices: " << num_indes << std::endl;

#if defined(PLATFORM_GLX)

	glGenVertexArrays(sizeof(g_vao) / sizeof(g_vao[0]), g_vao);

	for (unsigned i = 0; i < sizeof(g_vao) / sizeof(g_vao[0]); ++i)
		assert(g_vao[i]);

#endif

	glGenBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);

	for (unsigned i = 0; i < sizeof(g_vbo) / sizeof(g_vbo[0]); ++i)
		assert(g_vbo[i]);

#if defined(PLATFORM_GLX)

	glBindVertexArray(g_vao[PROG_SKIN]);

#endif

	glBindBuffer(GL_ARRAY_BUFFER, g_vbo[VBO_SKIN_VTX]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(arr), arr, GL_STATIC_DRAW);

	if (util::reportGLError())
	{
		std::cerr << __FUNCTION__ <<
			" failed at glBindBuffer/glBufferData for ARRAY_BUFFER" << std::endl;
		return false;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vbo[VBO_SKIN_IDX]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

	if (util::reportGLError())
	{
		std::cerr << __FUNCTION__ <<
			" failed at glBindBuffer/glBufferData for ELEMENT_ARRAY_BUFFER" << std::endl;
		return false;
	}

	if (!setupVertexAttrPointers< Vertex >(g_active_attr_semantics[PROG_SKIN])
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/////////////////////////////////////////////////////////////////

	static float anim = 0.f;

	rend::animateSkeleton(bone_count, g_bone_mat, g_bone, g_skeletal_animation, anim);

	anim += g_anim_step;

	if (1.f < anim)
	{
		anim -= floorf(anim);
		rend::resetSkeletonAnimProgress(g_skeletal_animation);
	}

	static const rend::matx4 mv = rend::matx4().translate(0.f, 0.f, -2.f);
	static const rend::matx4 pr = rend::matx4().persp(-.5f, .5f, -.5f, .5f, 1.f, 4.f);
	static rend::matx4 mvp = rend::matx4().mul(pr, mv).transpose(); // ES cannot transpose on the fly

	glUseProgram(g_shader_prog[PROG_SKIN]);

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKIN][UNI_MVP])
	{
		glUniformMatrix4fv(g_uni[PROG_SKIN][UNI_MVP],
			1, GL_FALSE, reinterpret_cast< GLfloat* >(mvp.dekast()));
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKIN][UNI_BONE])
	{
		glUniformMatrix4fv(g_uni[PROG_SKIN][UNI_BONE],
			bone_count, GL_FALSE, reinterpret_cast< GLfloat* >(g_bone_mat));
	}

	DEBUG_GL_ERR()

	if (-1 != g_uni[PROG_SKIN][UNI_LP_OBJ])
	{
		const GLfloat nonlocal_light[4] =
		{
			mv[2][0],
			mv[2][1],
			mv[2][2],
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

	if (g_tex[TEX_NORMAL] && -1 != g_uni[PROG_SKIN][UNI_SAMPLER_NORMAL])
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_NORMAL]);

		glUniform1i(g_uni[PROG_SKIN][UNI_SAMPLER_NORMAL], 0);
	}

	DEBUG_GL_ERR()

	if (g_tex[TEX_ALBEDO] && -1 != g_uni[PROG_SKIN][UNI_SAMPLER_ALBEDO])
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_tex[TEX_ALBEDO]);

		glUniform1i(g_uni[PROG_SKIN][UNI_SAMPLER_ALBEDO], 1);
	}

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKIN].num_active_attr; ++i)
		glEnableVertexAttribArray(g_active_attr_semantics[PROG_SKIN].active_attr[i]);

	DEBUG_GL_ERR()

	glDrawElements(GL_TRIANGLES, g_num_faces[MESH_SKIN] * 3, GL_UNSIGNED_SHORT, 0);

	DEBUG_GL_ERR()

	for (unsigned i = 0; i < g_active_attr_semantics[PROG_SKIN].num_active_attr; ++i)
		glDisableVertexAttribArray(g_active_attr_semantics[PROG_SKIN].active_attr[i]);

	DEBUG_GL_ERR()

	return true;
}

} // namespace testbed
