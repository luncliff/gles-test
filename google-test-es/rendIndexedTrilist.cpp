#if defined(PLATFORM_GLX)

#include <GL/gl.h>

#else

#include <GLES2/gl2.h>

#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits>
#include <iostream>
#include <iomanip>

#include "testbed.hpp"
#include "rendIndexedTrilist.hpp"


namespace testbed
{

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


template < typename T, unsigned NUM_T >
int fscanf_generic(
	FILE*,
	T (&)[NUM_T])
{
	assert(false);
	return 0;
}


template <>
int fscanf_generic(
	FILE* file,
	uint16_t (&out)[3])
{
	return fscanf(file, "%hu %hu %hu",
		&out[0],
		&out[1],
		&out[2]);
}


template <>
int fscanf_generic(
	FILE* file,
	uint32_t (&out)[3])
{
	return fscanf(file, "%u %u %u",
		&out[0],
		&out[1],
		&out[2]);
}


template <>
int fscanf_generic(
	FILE* file,
	float (&out)[6])

{
	return fscanf(file, "%f %f %f %f %f %f",
		&out[0],
		&out[1],
		&out[2],
		&out[3],
		&out[4],
		&out[5]);
}


template <>
int fscanf_generic(
	FILE* file,
	float (&out)[8])

{
	return fscanf(file, "%f %f %f %f %f %f %f %f",
		&out[0],
		&out[1],
		&out[2],
		&out[3],
		&out[4],
		&out[5],
		&out[6],
		&out[7]);
}


template < typename T >
const T& min(
	const T& a,
	const T& b)
{
	if (a < b)
		return a;

	return b;
}


template < typename T >
const T& max(
	const T& a,
	const T& b)
{
	if (a > b)
		return a;

	return b;
}


template <
	unsigned NUM_FLOATS_T,		// floats per vertex
	unsigned NUM_INDICES_T >	// indices per face
bool
fill_indexed_facelist_from_file(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	unsigned& num_faces,
	GLenum& index_type,
	const bool is_rotated)
{
	assert(filename);

	scoped_ptr< FILE, scoped_functor > file(fopen(filename, "r"));

	if (0 == file())
	{
		std::cerr << __FUNCTION__ << " failed at fopen '" << filename << "'" << std::endl;
		return false;
	}

	unsigned nv_total = 0;
	unsigned nf_total = 0;
	void *vb_total = 0;
	void *ib_total = 0;

	float vmin[3] =
	{
		std::numeric_limits< float >::infinity(),
		std::numeric_limits< float >::infinity(),
		std::numeric_limits< float >::infinity()
	};
	float vmax[3] =
	{
		-std::numeric_limits< float >::infinity(),
		-std::numeric_limits< float >::infinity(),
		-std::numeric_limits< float >::infinity()
	};

	typedef uint32_t BigIndex;
	typedef uint16_t CompactIndex;

	index_type = GL_UNSIGNED_INT; // BigIndex GL mapping
	const GLenum compact_index_type = GL_UNSIGNED_SHORT; // CompactIndex GL mapping

	while (true)
	{
		unsigned nv = 0;

		if (1 != fscanf(file(), "%u", &nv) || 0 == nv)
			break;

		scoped_ptr< void, generic_free > finish_vb(vb_total);
		scoped_ptr< void, generic_free > finish_ib(ib_total);

		if (uint64_t(nv) + nv_total > uint64_t(1) + BigIndex(-1))
		{
			std::cerr << __FUNCTION__ << " encountered too many vertices in '" <<
				filename << "'" << std::endl;
			return false;
		}

		finish_vb.reset();
		const size_t sizeof_vb = sizeof(float) * NUM_FLOATS_T * (nv + nv_total);
		scoped_ptr< void, generic_free > vb(realloc(vb_total, sizeof_vb));

		if (0 == vb())
			return false;

		for (unsigned i = 0; i < nv; ++i)
		{
			float (&vi)[NUM_FLOATS_T] =
				reinterpret_cast< float (*)[NUM_FLOATS_T] >(vb())[i + nv_total];

			if (NUM_FLOATS_T != fscanf_generic(file(), vi))
				return false;

			vmin[0] = min(vi[0], vmin[0]);
			vmin[1] = min(vi[1], vmin[1]);
			vmin[2] = min(vi[2], vmin[2]);

			vmax[0] = max(vi[0], vmax[0]);
			vmax[1] = max(vi[1], vmax[1]);
			vmax[2] = max(vi[2], vmax[2]);
		}

		unsigned nf = 0;

		if (1 != fscanf(file(), "%u", &nf) || 0 == nf)
			return false;

		finish_ib.reset();
		const size_t sizeof_ib = sizeof(BigIndex) * NUM_INDICES_T * (nf + nf_total);
		scoped_ptr< void, generic_free > ib(realloc(ib_total, sizeof_ib));

		if (0 == ib())
			return false;

		for (unsigned i = 0; i < nf; ++i)
		{
			BigIndex (&fi)[NUM_INDICES_T] =
				reinterpret_cast< BigIndex (*)[NUM_INDICES_T] >(ib())[i + nf_total];

			if (NUM_INDICES_T != fscanf_generic(file(), fi))
				return false;

			for (unsigned i = 0; i < NUM_INDICES_T; ++i)
				fi[i] += nv_total;
		}

		vb_total = vb();
		vb.reset();

		ib_total = ib();
		ib.reset();

		nv_total += nv;
		nf_total += nf;
	}

	if (0 == nv_total || 0 == nf_total)
		return false;

	std::cout << "number of vertices: " << nv_total <<
		"\nnumber of indices: " << nf_total * NUM_INDICES_T << std::endl;

	// normalize and re-center the mesh
	const float span = (vmax[0] - vmin[0]) > (vmax[1] - vmin[1])
		? ((vmax[0] - vmin[0]) > (vmax[2] - vmin[2]) ? vmax[0] - vmin[0] : vmax[2] - vmin[2])
		: ((vmax[1] - vmin[1]) > (vmax[2] - vmin[2]) ? vmax[1] - vmin[1] : vmax[2] - vmin[2]);

	const float origin[3] = {
		(vmin[0] + vmax[0]) * .5f,
		(vmin[1] + vmax[1]) * .5f,
		(vmin[2] + vmax[2]) * .5f
	};

	for (unsigned i = 0; i < nv_total; ++i)
	{
		float (&vi)[NUM_FLOATS_T] =
			reinterpret_cast< float (*)[NUM_FLOATS_T] >(vb_total)[i];

		vi[0] -= origin[0];
		vi[1] -= origin[1];
		vi[2] -= origin[2];

		vi[0] /= span * .5f;
		vi[1] /= span * .5f;
		vi[2] /= span * .5f;

		if (is_rotated)
		{
			const float vi_1 = -vi[1];
			vi[1] = vi[2];
			vi[2] = vi_1;

			const float vi_4 = -vi[4];
			vi[4] = vi[5];
			vi[5] = vi_4;
		}
	}

	size_t sizeof_index = sizeof(BigIndex);

	// compact index integral type if possible
	if (sizeof_index > sizeof(CompactIndex) &&
		uint64_t(1) + CompactIndex(-1) >= nv_total)
	{
		sizeof_index = sizeof(CompactIndex);

		const size_t sizeof_ib = sizeof_index * NUM_INDICES_T * nf_total;
		CompactIndex (* const ib)[NUM_INDICES_T] =
			reinterpret_cast< CompactIndex (*)[NUM_INDICES_T] >(malloc(sizeof_ib));

		if (0 == ib)
			return false;

		for (unsigned i = 0; i < nf_total; ++i)
		{
			BigIndex (&fi)[NUM_INDICES_T] =
				reinterpret_cast< BigIndex (*)[NUM_INDICES_T] >(ib_total)[i];

			for (unsigned j = 0; j < NUM_INDICES_T; ++j)
				ib[i][j] = CompactIndex(fi[j]);
		}

		free(ib_total);
		ib_total = ib;

		index_type = compact_index_type;
	}

	const size_t sizeof_vb = sizeof(float) * NUM_FLOATS_T * nv_total;
	const size_t sizeof_ib = sizeof_index * NUM_INDICES_T * nf_total;

	glBindBuffer(GL_ARRAY_BUFFER, vbo_arr);
	glBufferData(GL_ARRAY_BUFFER, sizeof_vb, vb_total, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_idx);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof_ib, ib_total, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	free(vb_total);
	free(ib_total);

	num_faces = nf_total;

	return true;
}

namespace util
{

bool
fill_indexed_trilist_from_file_PN(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	unsigned& num_faces,
	GLenum& index_type,
	const bool is_rotated)
{
	return fill_indexed_facelist_from_file< 6, 3 >(
		filename,
		vbo_arr,
		vbo_idx,
		num_faces,
		index_type,
		is_rotated);
}


bool
fill_indexed_trilist_from_file_PN2(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	unsigned& num_faces,
	GLenum& index_type,
	const bool is_rotated)
{
	return fill_indexed_facelist_from_file< 8, 3 >(
		filename,
		vbo_arr,
		vbo_idx,
		num_faces,
		index_type,
		is_rotated);
}


bool
fill_indexed_trilist_from_file_AGE(
	const char* const filename,
	const GLuint vbo_arr,
	const GLuint vbo_idx,
	const uintptr_t (&semantics_offset)[4],
	unsigned& num_faces,
	GLenum& index_type,
	float (&bmin)[3],
	float (&bmax)[3])
{
	assert(filename);

	scoped_ptr< FILE, scoped_functor > file(fopen(filename, "rb"));

	if (0 == file())
	{
		std::cerr << "error: failure at fopen '" << filename << "'" << std::endl;
		return false;
	}

	uint32_t magic;
	uint32_t version;

	if (1 != fread(&magic, sizeof(magic), 1, file()) ||
		1 != fread(&version, sizeof(version), 1, file()))
	{
		std::cerr << "error: failure at fread '" << filename << "'" << std::endl;
		return false;
	}

	std::cout << "mesh magic, version: 0x" << std::hex << std::setw(8) << std::setfill('0') <<
		magic << std::dec << ", " << version << std::endl;

	static const uint32_t sMagic = 0x6873656d;
	static const uint32_t sVersion = 100;

	if (magic != sMagic || version != sVersion)
	{
		std::cerr << "error: mesh of unknown magic/version" << std::endl;
		return false;
	}

	uint8_t softwareSkinning;
	uint16_t primitiveType;

	if (1 != fread(&softwareSkinning, sizeof(softwareSkinning), 1, file()) ||
		1 != fread(&primitiveType, sizeof(primitiveType), 1, file()))
	{
		std::cerr << "error: failure at fread '" << filename << "'" << std::endl;
		return false;
	}

	if (softwareSkinning)
	{
		std::cerr << "error: mesh uses software skinning" << std::endl;
		return false;
	}

	enum PrimitiveType
	{
		PT_TRIANGLE_LIST	= 4,
	};

	enum AttribType
	{
		AT_FLOAT1			= 0,
		AT_FLOAT2			= 1,
		AT_FLOAT3			= 2,
		AT_FLOAT4			= 3,
		AT_UBYTE4			= 9
	};

	enum AttribSemantic
	{
		AS_POSITION			= 1,
		AS_BLEND_WEIGHTS	= 2,
		AS_NORMAL			= 4,
		AS_COLOR			= 5,
		AS_TEXTURE_COORD	= 7,
		AS_BITANGENT		= 8,
		AS_TANGENT			= 9	
	};

	enum IndexType
	{
		INDEX_16BIT			= 0,
		INDEX_32BIT			= 1
	};

	const PrimitiveType type = (PrimitiveType) primitiveType;
	assert(type == PT_TRIANGLE_LIST); (void) type;

	uint16_t num_attr;

	if (1 != fread(&num_attr, sizeof(num_attr), 1, file()))
	{
		std::cerr << "error: failure at fread '" << filename << "'" << std::endl;
		return false;
	}

	std::cout << "vertex attributes: " << num_attr << std::endl;

	if (4 != num_attr)
	{
		std::cerr << "error: mesh of unsupported number of attributes" << std::endl;
		return false;
	}

	uint16_t buffer_interest = uint16_t(-1);

	for (unsigned i = 0; i < num_attr; ++i)
	{ 
		uint16_t src_buffer;
		uint16_t type;
		uint16_t semantic;
		uint16_t offset;
		uint16_t index;

		if (1 != fread(&src_buffer, sizeof(src_buffer), 1, file()) ||
			1 != fread(&type, sizeof(type), 1, file()) ||
			1 != fread(&semantic, sizeof(semantic), 1, file()) ||
			1 != fread(&offset, sizeof(offset), 1, file()) ||
			1 != fread(&index, sizeof(index), 1, file()))
		{
			std::cerr << "error: failure at fread '" << filename << "'" << std::endl;
			return false;
		}

		if (0 != index)
		{
			std::cerr << "error: mesh has secondary attributes" << std::endl;
			return false;
		}

		if (AS_POSITION == (AttribSemantic) semantic)
		{
			if (AT_FLOAT3 != (AttribType) type || semantics_offset[0] != offset ||
				(uint16_t(-1) != buffer_interest && buffer_interest != src_buffer))
			{
				std::cerr << "error: mesh has POSITION attribute of unsupported layout" << std::endl;
				return false;
			}
			buffer_interest = src_buffer;
			continue;
		}

		if (AS_BLEND_WEIGHTS == (AttribSemantic) semantic)
		{
			if (AT_FLOAT4 != (AttribType) type || semantics_offset[1] != offset ||
				(uint16_t(-1) != buffer_interest && buffer_interest != src_buffer))
			{
				std::cerr << "error: mesh has BLEND_WEIGHTS attribute of unsupported layout" << std::endl;
				return false;
			}
			buffer_interest = src_buffer;
			continue;
		}

		if (AS_NORMAL == (AttribSemantic) semantic)
		{
			if (AT_FLOAT3 != (AttribType) type || semantics_offset[2] != offset ||
				(uint16_t(-1) != buffer_interest && buffer_interest != src_buffer))
			{
				std::cerr << "error: mesh has NORMAL attribute of unsupported layout" << std::endl;
				return false;
			}
			buffer_interest = src_buffer;
			continue;
		}

		if (AS_TEXTURE_COORD == (AttribSemantic) semantic)
		{
			if (AT_FLOAT2 != (AttribType) type || semantics_offset[3] != offset ||
				(uint16_t(-1) != buffer_interest && buffer_interest != src_buffer))
			{
				std::cerr << "error: mesh has TEXTURE_COORD attribute of unsupported layout" << std::endl;
				return false;
			}
			buffer_interest = src_buffer;
			continue;
		}

		std::cerr << "error: mesh has attribute of unsupported semantics; bailing out" << std::endl;
		return false;
	}

	if (uint16_t(-1) == buffer_interest)
	{
		std::cerr << "error: mesh has no usable attribute buffer; bailing out" << std::endl;
		return false;
	}

	uint32_t num_vertices;
	uint16_t num_buffers;

	if (1 != fread(&num_vertices, sizeof(num_vertices), 1, file()) ||
		1 != fread(&num_buffers, sizeof(num_buffers), 1, file()))
	{
		std::cerr << "error: failure at fread '" << filename << "'" << std::endl;
		return false;
	}

	size_t sizeof_vb = 0;
	void* proto_vb = 0;

	for (unsigned i = 0; i < num_buffers; ++i)
	{ 
		uint16_t bind_index;
		uint16_t vertex_size;

		if (1 != fread(&bind_index, sizeof(bind_index), 1, file()) ||
			1 != fread(&vertex_size, sizeof(vertex_size), 1, file()))
		{
			std::cerr << "error: failure at fread '" << filename << "'" << std::endl;
			return false;
		}

		sizeof_vb = size_t(vertex_size) * num_vertices;

		if (buffer_interest != i)
		{
			fseek(file(), sizeof_vb, SEEK_CUR);
			continue;
		}

		scoped_ptr< void, generic_free > buf(malloc(sizeof_vb));

		if (0 == buf() || 1 != fread(buf(), sizeof_vb, 1, file()))
		{
			std::cerr << "error: failure reading attribute buffer from file '" << filename << "'" << std::endl;
			return false;
		}

		proto_vb = buf();
		buf.reset();
	}

	scoped_ptr< void, generic_free > vb(proto_vb);
	proto_vb = 0;

	uint16_t index_format;
	uint32_t num_indices;

	if (1 != fread(&index_format, sizeof(index_format), 1, file()) ||
		1 != fread(&num_indices, sizeof(num_indices), 1, file()))
	{
		std::cerr << "error: failure at fread '" << filename << "'" << std::endl;
		return false;
	}

	if (0 == num_indices)
	{
		std::cerr << "error: mesh not indexed" << std::endl;
		return false;
	}

	size_t sizeof_index = 0;

	switch ((IndexType) index_format)
	{
	case INDEX_16BIT:
		index_type = GL_UNSIGNED_SHORT;
		sizeof_index = sizeof(uint16_t);
		break;

	case INDEX_32BIT:
		index_type = GL_UNSIGNED_INT;
		sizeof_index = sizeof(uint32_t);
		break;

	default:
		std::cerr << "error: unsupported index format" << std::endl;
		return false;
	}

	const size_t sizeof_ib = sizeof_index * num_indices;
	scoped_ptr< void, generic_free > ib(malloc(sizeof_ib));

	if (0 == ib() || 1 != fread(ib(), sizeof_ib, 1, file()))
	{
		std::cerr << "error: failure reading index buffer from file '" << filename << "'" << std::endl;
		return false;
	}

	if (1 != fread(&bmin, sizeof(bmin), 1, file()) ||
		1 != fread(&bmax, sizeof(bmax), 1, file()))
	{
		std::cerr << "error: failure at fread '" << filename << "'" << std::endl;
		return false;
	}

	std::cout << "bbox min: (" <<
		bmin[0] << ", " <<
		bmin[1] << ", " <<
		bmin[2] << ")\nbbox max: (" <<
		bmax[0] << ", " <<
		bmax[1] << ", " <<
		bmax[2] << ")" << std::endl;	

	float center[3];
	float radius;

	if (1 != fread(&center, sizeof(center), 1, file()) ||
		1 != fread(&radius, sizeof(radius), 1, file()))
	{
		std::cerr << "error: failure at fread '" << filename << "'" << std::endl;
		return false;
	}

	std::cout << "number of vertices: " << num_vertices <<
		"\nnumber of indices: " << num_indices << std::endl;

	glBindBuffer(GL_ARRAY_BUFFER, vbo_arr);
	glBufferData(GL_ARRAY_BUFFER, sizeof_vb, vb(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_idx);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof_ib, ib(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	num_faces = num_indices / 3;

	return true;
}

} // namespace util
} // namespace testbed
