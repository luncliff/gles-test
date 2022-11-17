#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglfslext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2amdext.h>
#include "pure_macro.hpp"

#if EGL_KHR_image == 0
#error Missing required extension EGL_KHR_image.
#endif

#if EGL_FSL_create_image == 0
#error Missing required extension EGL_FSL_create_image.
#endif

#if EGL_QUERY_IMAGE_FSL == 0
#error Missing required extension EGL_QUERY_IMAGE_FSL.
#endif

#if GL_OES_EGL_image_external == 0
#error Missing required extension GL_OES_EGL_image_external.
#endif

#if GL_AMD_EGL_image_external_layout_specifier == 0
#error Missing required extension GL_AMD_EGL_image_external_layout_specifier.
#endif

#if GL_NV_fence == 0
#error Missing required extension GL_NV_fence.
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <iostream>

#include "utilPix.hpp"
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

static const char arg_image_dim[]	= "image_dim";
static const char arg_static[]		= "static";

static unsigned g_image_w = 256;
static unsigned g_image_h = 256;
static bool g_static;

static EGLDisplay g_display = EGL_NO_DISPLAY;
static EGLContext g_context = EGL_NO_CONTEXT;

static EGLImageKHR g_image[] =
{
	EGL_NO_IMAGE_KHR,
	EGL_NO_IMAGE_KHR
};
static const unsigned num_images = sizeof(g_image) / sizeof(g_image[0]);
static EGLImageInfoFSL g_client_buffer[num_images];
static GLuint g_fence[num_images];

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

				if (!strcmp(option, arg_static))
				{
					g_static = true;
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
			" <width> <height>\t: use specified dimensions with source image; width must be a multiple of 64, height must be a multiple of 2\n"
			"\t" << testbed::arg_prefix << testbed::arg_app << " " << arg_static <<
			"\t\t\t: disable image update at each frame\n" << std::endl;
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
		if (EGL_NO_IMAGE_KHR != g_image[i])
		{
			const EGLImageInfoFSL nil_info =
			{
				{ 0, 0, 0 },
				{ 0, 0, 0 },
				0
			};

			eglDestroyImageKHR(g_display, g_image[i]);
			g_image[i] = EGL_NO_IMAGE_KHR;
			g_client_buffer[i] = nil_info;
		}

	glDeleteFencesNV(sizeof(g_fence) / sizeof(g_fence[0]), g_fence);
	memset(g_fence, 0, sizeof(g_fence));

	glDeleteTextures(sizeof(g_tex) / sizeof(g_tex[0]), g_tex);
	memset(g_tex, 0, sizeof(g_tex));

	glDeleteBuffers(sizeof(g_vbo) / sizeof(g_vbo[0]), g_vbo);
	memset(g_vbo, 0, sizeof(g_vbo));

	g_display = EGL_NO_DISPLAY;
	g_context = EGL_NO_CONTEXT;

	return true;
}


template < typename T >
class generic_free
{
public:

	void operator()(T* arg)
	{
		free(arg);
	}
};


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

	util::pix* tex_src = 0;

	if (g_static)
	{
		const unsigned tex_size = g_image_h * g_image_w * sizeof(util::pix);

		// provide some guardband as pixels are of non-word-multiple size
		tex_src = (util::pix*) malloc(util::next_multiple_of_pix_integral(tex_size));

		if (0 == tex_src)
		{
			std::cerr << __FUNCTION__ << " failed at malloc" << std::endl;
			return false;
		}

		util::fill_with_checker(tex_src, g_image_w * sizeof(util::pix), g_image_w, g_image_h);
	}
	else
	{
		glGenFencesNV(sizeof(g_fence) / sizeof(g_fence[0]), g_fence);

		for (unsigned i = 0; i < sizeof(g_fence) / sizeof(g_fence[0]); ++i)
		{
			assert(g_fence[i]);

			// emit a dummy fence for each fence name so that the
			// initial fence wait would not require special handing
			glSetFenceNV(g_fence[i], GL_ALL_COMPLETED_NV);
		}
	}

	scoped_ptr< util::pix, generic_free > finish_tex_src(tex_src);

	glGenTextures(sizeof(g_tex) / sizeof(g_tex[0]), g_tex);

	for (unsigned i = 0; i < sizeof(g_tex) / sizeof(g_tex[0]); ++i)
		assert(g_tex[i]);

	for (unsigned i = 0; i < num_images; ++i)
	{
		const EGLint attr[] =
		{
			EGL_WIDTH, g_image_w,
			EGL_HEIGHT, g_image_h,
			EGL_IMAGE_FORMAT_FSL, EGL_FORMAT_YUV_YV12_FSL,
			EGL_NONE
		};

		g_image[i] = eglCreateImageKHR(g_display, EGL_NO_CONTEXT, EGL_NEW_IMAGE_FSL, NULL, attr);

		if (EGL_NO_IMAGE_KHR == g_image[i])
		{
			std::cerr << __FUNCTION__ << " failed at eglCreateImageKHR" << std::endl;
			return false;
		}

		eglQueryImageFSL(g_display, g_image[i], EGL_CLIENTBUFFER_TYPE_FSL,
			reinterpret_cast< EGLint* >(&g_client_buffer[i]));

		glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_tex[i]);
		glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, g_image[i]);

		GLint param = 0;

		glGetTexParameteriv(
			GL_TEXTURE_EXTERNAL_OES,
			GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES,
			&param);

		glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

		std::cout << "image sibling of " << param << " texture image unit(s)" << std::endl;

		if (g_static)
		{
			util::fill_YUV420_from_RGB(
				(uint8_t*) g_client_buffer[i].mem_virt[0],
				(uint8_t*) g_client_buffer[i].mem_virt[1],
				(uint8_t*) g_client_buffer[i].mem_virt[2],
				g_image_w, g_image_w / 2, g_image_w / 2,
				tex_src,
				g_image_w * sizeof(util::pix),
				g_image_w, g_image_h);
		}
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

	if (!util::setupShader(g_shader_frag[PROG_QUAD], "texture_external_yv12.glslf"))
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
		if (!g_static)
		{
			glFinishFenceNV(g_fence[idx]);

			static uint8_t grad;
			memset(g_client_buffer[idx].mem_virt[0], grad++, g_image_w * g_image_h);
			memset(g_client_buffer[idx].mem_virt[1], grad++, g_image_w * g_image_h / 4);
			memset(g_client_buffer[idx].mem_virt[2], grad++, g_image_w * g_image_h / 4);
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_tex[idx]);

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

	if (!g_static)
		glSetFenceNV(g_fence[idx], GL_ALL_COMPLETED_NV);

	DEBUG_GL_ERR()

	return true;
}

} // namespace testbed
