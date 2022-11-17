/* This header can be included only after rendVertAttr.hpp */

#ifndef rend_vert_attr_H__
#error rendVertAttr.hpp needs to be included first
#endif

#define SETUP_VERTEX_ATTR_POINTERS_MASK_vertex		0x00000001
#define SETUP_VERTEX_ATTR_POINTERS_MASK_normal		0x00000002
#define SETUP_VERTEX_ATTR_POINTERS_MASK_blendw		0x00000004
#define SETUP_VERTEX_ATTR_POINTERS_MASK_tcoord		0x00000008
#define SETUP_VERTEX_ATTR_POINTERS_MASK_index		0x00000010
#define SETUP_VERTEX_ATTR_POINTERS_MASK_vert2d		0x00000020

#if SETUP_VERTEX_ATTR_POINTERS_MASK == 0
#error SETUP_VERTEX_ATTR_POINTERS_MASK missing or nil
#endif

#if (SETUP_VERTEX_ATTR_POINTERS_MASK & SETUP_VERTEX_ATTR_POINTERS_MASK_vertex) && \
	(SETUP_VERTEX_ATTR_POINTERS_MASK & SETUP_VERTEX_ATTR_POINTERS_MASK_vert2d)
#error SETUP_VERTEX_ATTR_POINTERS_MASK contains both vertex & vert2d
#endif

#if SETUP_VERTEX_ATTR_POINTERS_MASK & SETUP_VERTEX_ATTR_POINTERS_MASK_vert2d
#define VERTEX_ATTR_POS_DIM 2
#else
#define VERTEX_ATTR_POS_DIM 3
#endif

template < class VERTEX_T >
static bool
setupVertexAttrPointers(
	const rend::ActiveAttrSemantics& active_attr_semantics,
	const uintptr_t va = 0)
{
#if (SETUP_VERTEX_ATTR_POINTERS_MASK & SETUP_VERTEX_ATTR_POINTERS_MASK_vertex) || \
	(SETUP_VERTEX_ATTR_POINTERS_MASK & SETUP_VERTEX_ATTR_POINTERS_MASK_vert2d)

	if (active_attr_semantics.semantics_vertex != -1)
	{
		const float (VERTEX_T::* const offs)[VERTEX_ATTR_POS_DIM] = &VERTEX_T::pos;

		glVertexAttribPointer(active_attr_semantics.getVertexAttr(), VERTEX_ATTR_POS_DIM, GL_FLOAT, GL_FALSE, sizeof(VERTEX_T),
			*reinterpret_cast< const int8_t* const* >(&offs) + va);

		DEBUG_GL_ERR()
	}

#else
	assert(active_attr_semantics.semantics_vertex == -1);

#endif

#if SETUP_VERTEX_ATTR_POINTERS_MASK & SETUP_VERTEX_ATTR_POINTERS_MASK_normal

	if (active_attr_semantics.semantics_normal != -1)
	{
		const float (VERTEX_T::* const offs)[3] = &VERTEX_T::nrm;

		glVertexAttribPointer(active_attr_semantics.getNormalAttr(), 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX_T),
			*reinterpret_cast< const int8_t* const* >(&offs) + va);

		DEBUG_GL_ERR()
	}

#else
	assert(active_attr_semantics.semantics_normal == -1);

#endif

#if SETUP_VERTEX_ATTR_POINTERS_MASK & SETUP_VERTEX_ATTR_POINTERS_MASK_blendw

	if (active_attr_semantics.semantics_blendw != -1)
	{
		const float (VERTEX_T::* const offs)[4] = &VERTEX_T::bon;

		glVertexAttribPointer(active_attr_semantics.getBlendWAttr(), 4, GL_FLOAT, GL_FALSE, sizeof(VERTEX_T),
			*reinterpret_cast< const int8_t* const* >(&offs) + va);

		DEBUG_GL_ERR()
	}

#else
	assert(active_attr_semantics.semantics_blendw == -1);

#endif

#if SETUP_VERTEX_ATTR_POINTERS_MASK & SETUP_VERTEX_ATTR_POINTERS_MASK_tcoord

	if (active_attr_semantics.semantics_tcoord != -1)
	{
		const float (VERTEX_T::* const offs)[2] = &VERTEX_T::txc;

		glVertexAttribPointer(active_attr_semantics.getTCoordAttr(), 2, GL_FLOAT, GL_FALSE, sizeof(VERTEX_T),
			*reinterpret_cast< const int8_t* const* >(&offs) + va);

		DEBUG_GL_ERR()
	}

#else
#if SETUP_VERTEX_ATTR_POINTERS_MASK & SETUP_VERTEX_ATTR_POINTERS_MASK_vertex
	// When texcoord is expected but the vertex type does not supply the semantics, substitute for pos.xy
	if (active_attr_semantics.semantics_tcoord != -1)
	{
		const float (VERTEX_T::* const offs)[VERTEX_ATTR_POS_DIM] = &VERTEX_T::pos;

		glVertexAttribPointer(active_attr_semantics.getTCoordAttr(), 2, GL_FLOAT, GL_FALSE, sizeof(VERTEX_T),
			*reinterpret_cast< const int8_t* const* >(&offs) + va);

		DEBUG_GL_ERR()
	}

#else
	assert(active_attr_semantics.semantics_tcoord == -1);

#endif
#endif

#if SETUP_VERTEX_ATTR_POINTERS_MASK & SETUP_VERTEX_ATTR_POINTERS_MASK_index

	if (active_attr_semantics.semantics_index != -1)
	{
		const uint16_t (VERTEX_T::* const offs)[1] = &VERTEX_T::idx;

		glVertexAttribPointer(active_attr_semantics.getIndexAttr(), 1, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(VERTEX_T),
			*reinterpret_cast< const int8_t* const* >(&offs) + va);

		DEBUG_GL_ERR()
	}

#else
	assert(active_attr_semantics.semantics_index == -1);

#endif

	return true;
}

