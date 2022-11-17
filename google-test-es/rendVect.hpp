#ifndef rend_vect_H__
#define rend_vect_H__

#include <cmath>
#include <cstring>
#include <cassert>

namespace rend
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// remark on conventions
//
// vectors are taken as column-vectors and matrices as column-major, thus matrix transformations
// follow operator-leftism, and sequences of transformations are written out as
//
// Mn * Mn-1 * .. * M1 * M0 * v
//
// the terms sub- and super-dimensionality signify respectively less-or-equal
// and greater-or-equal dimensionality than that of the class at hand
//
// all mutators return the mutated object, and all accessors taking an output parameter return said
// parameter, as in
//
// mutated_type& setAttribute(attribute_type& in)
// {
//     ...
//	   return *this;
// }
//
// attribute_type& getAttribute(attribute_type& out) const
// {
//     ...
//	   return out;
// }
//
////////////////////////////////////////////////////////////////////////////////////////////////////

template < unsigned DIMENSION_T,
		   unsigned STRIDE0_T,
		   unsigned STRIDE1_T,
		   typename SCALTYPE_T >
inline void
sparse_copy(
	SCALTYPE_T* const d,
	const SCALTYPE_T* const s)
{
	assert(0 != DIMENSION_T);
	assert(0 != STRIDE0_T && 0 != STRIDE1_T);

	if (1 == STRIDE0_T &&
		1 == STRIDE1_T)
	{
		memcpy(d, s, sizeof(SCALTYPE_T) * DIMENSION_T);
		return;
	}

	for (unsigned i = 0; i < DIMENSION_T; ++i)
		d[STRIDE0_T * i] = s[STRIDE1_T * i];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class protovector
// type- and dimensionality-agnostic vector, offering some typecasting and a basic set of vector ops
// warning: do not use directly, subclass like this:
//
//			class myvect : public protovect< myScalarType, myDimension, myvect >
//
//			where myvect does not introduce any new data members!
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
class protovect
{
protected:

	SCALTYPE_T m[DIMENSION_T];

public:

	enum { dimension = DIMENSION_T };
	typedef SCALTYPE_T scaltype;
	typedef SUBCLASS_T subclass;

	protovect()
	{}

	////////////////////////////////////////////////////////////////////////////////////////////////
	static SUBCLASS_T& cast(
		SCALTYPE_T (&v)[DIMENSION_T]);

	static const SUBCLASS_T& cast(
		const SCALTYPE_T (&v)[DIMENSION_T]);

	template < unsigned SUPERDIM_T >
	static SUBCLASS_T& castU(
		SCALTYPE_T (&v)[SUPERDIM_T]);

	template < unsigned SUPERDIM_T >
	static const SUBCLASS_T& castU(
		const SCALTYPE_T (&v)[SUPERDIM_T]);

	////////////////////////////////////////////////////////////////////////////////////////////////
	typedef const SCALTYPE_T (& decast)[DIMENSION_T];

	operator decast () const															// implicit typecast to 'const SCALTYPE_T[DIMENSION_T]'
		{ return m; }

	SCALTYPE_T (& dekast())[DIMENSION_T]												// explicit typecast to 'SCALTYPE_T[DIMENSION_T]'
		{ return m; }

	////////////////////////////////////////////////////////////////////////////////////////////////
	operator SUBCLASS_T&();

	operator const SUBCLASS_T&() const;

	bool operator ==(
		const protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >& v) const;

	bool operator !=(
		const protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >& v) const;

	////////////////////////////////////////////////////////////////////////////////////////////////
	template < class ANYCLASS_T >
	SUBCLASS_T& set(																	// bulk mutator
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v);

	SUBCLASS_T& set(																	// element mutator
		const unsigned i,
		const SCALTYPE_T c);

	SCALTYPE_T get(																		// element accessor
		const unsigned i) const;

	////////////////////////////////////////////////////////////////////////////////////////////////
	SUBCLASS_T& negate();																// negate this

	template < class ANYCLASS_T >
	SUBCLASS_T& negate(																	// negative of argument
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v);

	SUBCLASS_T& mul(																	// product of this and scalar argument
		const SCALTYPE_T c);

	template < class ANYCLASS_T >
	SUBCLASS_T& mul(																	// product of argument and scalar argument
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v,
		const SCALTYPE_T c);

	template < class ANYCLASS_T >
	SUBCLASS_T& add(																	// sum of this and argument
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v);

	template < class ANYCLASS0_T, class ANYCLASS1_T >
	SUBCLASS_T& add(																	// sum of arguments
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS0_T >& v0,
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS1_T >& v1);

	template < class ANYCLASS_T >
	SUBCLASS_T& sub(																	// difference between this and argument
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v);

	template < class ANYCLASS0_T, class ANYCLASS1_T >
	SUBCLASS_T& sub(																	// difference between arguments
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS0_T >& v0,
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS1_T >& v1);

	template < class ANYCLASS_T >
	SUBCLASS_T& mul(																	// element-wise multiplication with argument
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v);

	template < class ANYCLASS0_T, class ANYCLASS1_T >
	SUBCLASS_T& mul(																	// element-wise multiplication of arguments
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS0_T >& v0,
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS1_T >& v1);

	template < class ANYCLASS_T >
	SUBCLASS_T& div(																	// element-wise division by argument
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v);

	template < class ANYCLASS0_T, class ANYCLASS1_T >
	SUBCLASS_T& div(																	// element-wise division of first by second argument
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS0_T >& v0,
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS1_T >& v1);

	template < class ANYCLASS_T >
	SUBCLASS_T& wsum(																	// weighted sum of this and argument
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v,
		const SCALTYPE_T factor0,
		const SCALTYPE_T factor1);

	template < class ANYCLASS0_T, class ANYCLASS1_T >
	SUBCLASS_T& wsum(																	// weighted sum of arguments
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS0_T >& v0,
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS1_T >& v1,
		const SCALTYPE_T factor0,
		const SCALTYPE_T factor1);
};


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::cast(
	SCALTYPE_T (&v)[DIMENSION_T])
{
	return reinterpret_cast< SUBCLASS_T& >(v);
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
inline const SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::cast(
	const SCALTYPE_T (&v)[DIMENSION_T])
{
	return reinterpret_cast< const SUBCLASS_T& >(v);
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < unsigned SUPERDIM_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::castU(
	SCALTYPE_T (&v)[SUPERDIM_T])
{
	assert(SUPERDIM_T >= DIMENSION_T);

	return reinterpret_cast< SUBCLASS_T& >(v);
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < unsigned SUPERDIM_T >
inline const SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::castU(
	const SCALTYPE_T (&v)[SUPERDIM_T])
{
	assert(SUPERDIM_T >= DIMENSION_T);

	return reinterpret_cast< const SUBCLASS_T& >(v);
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
inline protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::operator SUBCLASS_T&()
{
	return *static_cast< SUBCLASS_T* >(this);
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
inline protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::operator const SUBCLASS_T&() const
{
	return *static_cast< const SUBCLASS_T* >(this);
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
inline bool protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::operator ==(
	const protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >& v) const
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m[i] != v.m[i])
			return false;

	return true;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
inline bool protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::operator !=(
	const protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >& v) const
{
	return !operator ==(v);
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::set(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v)
{
	return *this = cast(v);
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::set(
	const unsigned i,
	const SCALTYPE_T c)
{
	assert(i < DIMENSION_T);

	m[i] = c;

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
inline SCALTYPE_T
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::get(
	const unsigned i) const
{
	assert(i < DIMENSION_T);

	return m[i];
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::mul(
	const SCALTYPE_T c)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] *= c;

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::mul(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v,
	const SCALTYPE_T c)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] = v[i] * c;

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::negate()
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] = -m[i];

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::negate(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] = -v[i];

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::add(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] += v[i];

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS0_T, class ANYCLASS1_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::add(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS0_T >& v0,
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS1_T >& v1)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] = v0[i] + v1[i];

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::sub(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] -= v[i];

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS0_T, class ANYCLASS1_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::sub(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS0_T >& v0,
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS1_T >& v1)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] = v0[i] - v1[i];

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::mul(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] *= v[i];

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS0_T, class ANYCLASS1_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::mul(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS0_T >& v0,
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS1_T >& v1)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] = v0[i] * v1[i];

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::div(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] /= v[i];

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS0_T, class ANYCLASS1_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::div(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS0_T >& v0,
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS1_T >& v1)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] = v0[i] / v1[i];

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::wsum(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v,
	const SCALTYPE_T factor0,
	const SCALTYPE_T factor1)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] = m[i] * factor0 + v[i] * factor1;

	return *this;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS0_T, class ANYCLASS1_T >
inline SUBCLASS_T&
protovect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::wsum(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS0_T >& v0,
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS1_T >& v1,
	const SCALTYPE_T factor0,
	const SCALTYPE_T factor1)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		m[i] = v0[i] * factor0 + v1[i] * factor1;

	return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect_generic
// minimalist instantiatable specialization of protovect; chances are you want to use something else
// as one should normally aim for the most derived vector subclass that fits a given scenario
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename SCALTYPE_T, unsigned DIMENSION_T >
class vect_generic : public protovect< SCALTYPE_T, DIMENSION_T, vect_generic< SCALTYPE_T, DIMENSION_T > >
{
public:

	vect_generic()
	{}

	explicit vect_generic(
		const SCALTYPE_T (&v)[DIMENSION_T]);
};


template < typename SCALTYPE_T, unsigned DIMENSION_T >
inline vect_generic< SCALTYPE_T, DIMENSION_T >::vect_generic(
	const SCALTYPE_T (&v)[DIMENSION_T])
{
	*this = this->cast(v);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vectz
// integer-based dimensionality-agnostic vector; hosts integer-only functionality
////////////////////////////////////////////////////////////////////////////////////////////////////

template < unsigned DIMENSION_T, class SUBCLASS_T >
class vectz : public protovect< int, DIMENSION_T, SUBCLASS_T >
{
public:

	vectz()
	{}

	explicit vectz(
		const int (&v)[DIMENSION_T]);

	////////////////////////////////////////////////////////////////////////////////////////////////
	operator SUBCLASS_T&()
		{ return *static_cast< SUBCLASS_T* >(this); }

	operator const SUBCLASS_T&() const
		{ return *static_cast< const SUBCLASS_T* >(this); }
};


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline vectz< DIMENSION_T, SUBCLASS_T >::vectz(
	const int (&v)[DIMENSION_T])
{
	*this = this->cast(v);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class ivect
// convenience wrapper of class vectz
////////////////////////////////////////////////////////////////////////////////////////////////////

template < unsigned DIMENSION_T >
class ivect : public vectz< DIMENSION_T, ivect< DIMENSION_T > >
{
public:

	ivect()
	{}

	explicit ivect(
		const int (&v)[DIMENSION_T]);
};


template < unsigned DIMENSION_T >
inline ivect< DIMENSION_T >::ivect(
	const int (&v)[DIMENSION_T])
: vectz< DIMENSION_T, ivect< DIMENSION_T > >(v)
{}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class ivect<2>
// specialization of ivect for DIMENSION_T = 2
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class ivect< 2 > : public vectz< 2, ivect< 2 > >
{
public:

	ivect()
	{}

	explicit ivect(
		const int (&v)[2]);

	explicit ivect(
		const int c0,
		const int c1);
};


inline ivect< 2 >::ivect(
	const int (&v)[2])
: vectz< 2, ivect< 2 > >(v)
{}


inline ivect< 2 >::ivect(
	const int c0,
	const int c1)
{
	set(0, c0);
	set(1, c1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class ivect<3>
// specialization of ivect for DIMENSION_T = 3
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class ivect< 3 > : public vectz< 3, ivect< 3 > >
{
public:

	ivect()
	{}

	explicit ivect(
		const int (&v)[3]);

	explicit ivect(
		const int c0,
		const int c1,
		const int c2);
};


inline ivect< 3 >::ivect(
	const int (&v)[3])
: vectz< 3, ivect< 3 > >(v)
{}


inline ivect< 3 >::ivect(
	const int c0,
	const int c1,
	const int c2)
{
	set(0, c0);
	set(1, c1);
	set(2, c2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class ivect<4>
// specialization of ivect for DIMENSION_T = 4
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class ivect< 4 > : public vectz< 4, ivect< 4 > >
{
public:

	ivect()
	{}

	explicit ivect(
		const int (&v)[4]);

	explicit ivect(
		const int c0,
		const int c1,
		const int c2,
		const int c3);
};


inline ivect< 4 >::ivect(
	const int (&v)[4])
: vectz< 4, ivect< 4 > >(v)
{}


inline ivect< 4 >::ivect(
	const int c0,
	const int c1,
	const int c2,
	const int c3)
{
	set(0, c0);
	set(1, c1);
	set(2, c2);
	set(3, c3);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// prerequisites for real-based vectors
////////////////////////////////////////////////////////////////////////////////////////////////////

template < unsigned DIMENSION_T,
		   unsigned STRIDE0_T,
		   unsigned STRIDE1_T >
inline float
dot(
	const float* const v0,
	const float* const v1)
{
	assert(0 != DIMENSION_T);
	assert(0 != STRIDE0_T && 0 != STRIDE1_T);

#ifdef REND_MADD__

	float r = v0[0] * v1[0];

	for (unsigned i = 1; i < DIMENSION_T; ++i)
		r += v0[STRIDE0_T * i] * v1[STRIDE1_T * i];

	return r;

#else

	float r[DIMENSION_T];

	for (unsigned i = 0; i < DIMENSION_T; ++i)
		r[i] = v0[STRIDE0_T * i] * v1[STRIDE1_T * i];

	for (unsigned i = 1; i < DIMENSION_T; ++i)
		r[0] += r[i];

	return r[0];

#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vectr
// real-based dimensionality-agnostic vector; hosts real-only functionality
////////////////////////////////////////////////////////////////////////////////////////////////////

template < unsigned DIMENSION_T, class SUBCLASS_T >
class vectr : public protovect< float, DIMENSION_T, SUBCLASS_T >
{
public:

	vectr()
	{}

	explicit vectr(
		const float (&v)[DIMENSION_T]);

	////////////////////////////////////////////////////////////////////////////////////////////////
	operator SUBCLASS_T&()
		{ return *static_cast< SUBCLASS_T* >(this); }

	operator const SUBCLASS_T&() const
		{ return *static_cast< const SUBCLASS_T* >(this); }

	////////////////////////////////////////////////////////////////////////////////////////////////
	template < class ANYCLASS_T >
	float dot(																			// dot product
		const vectr< DIMENSION_T, ANYCLASS_T >& v) const;

	float sqr() const;																	// Euclidean norm, squared

	float norm() const;																	// Euclidean norm

	////////////////////////////////////////////////////////////////////////////////////////////////
	SUBCLASS_T& normalise();															// normalised this

	template < class ANYCLASS_T >
	SUBCLASS_T& normalise(																// normalised argument
		const vectr< DIMENSION_T, ANYCLASS_T >& v);

	template < class ANYCLASS0_T, class ANYCLASS1_T >
	SUBCLASS_T& interpolate(															// distance-based interpolation between arguments
		const vectr< DIMENSION_T, ANYCLASS0_T >& v0,
		const vectr< DIMENSION_T, ANYCLASS1_T >& v1,
		float dist0,
		float dist1);
};


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline vectr< DIMENSION_T, SUBCLASS_T >::vectr(
	const float (&v)[DIMENSION_T])
{
	*this = this->cast(v);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline float
vectr< DIMENSION_T, SUBCLASS_T >::dot(
	const vectr< DIMENSION_T, ANYCLASS_T >& v) const
{
	return rend::dot< DIMENSION_T, 1, 1 >(this->m, v);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline float
vectr< DIMENSION_T, SUBCLASS_T >::sqr() const
{
	return dot(*this);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline float
vectr< DIMENSION_T, SUBCLASS_T >::norm() const
{
	return sqrt(sqr());
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline SUBCLASS_T&
vectr< DIMENSION_T, SUBCLASS_T >::normalise()
{
	return this->mul(1.f / norm());
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
vectr< DIMENSION_T, SUBCLASS_T >::normalise(
	const vectr< DIMENSION_T, ANYCLASS_T >& v)
{
	return this->mul(v, 1.f / v.norm());
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS0_T, class ANYCLASS1_T >
inline SUBCLASS_T&
vectr< DIMENSION_T, SUBCLASS_T >::interpolate(
	const vectr< DIMENSION_T, ANYCLASS0_T >& v0,
	const vectr< DIMENSION_T, ANYCLASS1_T >& v1,
	float dist0,
	float dist1)
{
	const float d0 = fabs(dist0);
	const float d1 = fabs(dist1);

	const float f0 = d1 / (d0 + d1);
	const float f1 = d0 / (d0 + d1);

	return this->wsum(v0, v1, f0, f1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect
// convenience wrapper of class vectr
////////////////////////////////////////////////////////////////////////////////////////////////////

template < unsigned DIMENSION_T >
class vect : public vectr< DIMENSION_T, vect< DIMENSION_T > >
{
public:

	vect()
	{}

	explicit vect(
		const float (&v)[DIMENSION_T]);
};


template < unsigned DIMENSION_T >
inline vect< DIMENSION_T >::vect(
	const float (&v)[DIMENSION_T])
: vectr< DIMENSION_T, vect< DIMENSION_T > >(v)
{}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect<2>
// specialization of vect for DIMENSION_T = 2
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class vect< 2 > : public vectr< 2, vect< 2 > >
{
public:

	vect()
	{}

	explicit vect(
		const float (&v)[2]);

	explicit vect(
		const float c0,
		const float c1);

	////////////////////////////////////////////////////////////////////////////////////////////////
	template < class ANYCLASS_T >
	float cross(														// cross-product of this and argument
		const vectr< 2, ANYCLASS_T >& v) const;
};


inline vect< 2 >::vect(
	const float (&v)[2])
: vectr< 2, vect< 2 > >(v)
{}


inline vect< 2 >::vect(
	const float c0,
	const float c1)
{
	set(0, c0);
	set(1, c1);
}


template < class ANYCLASS_T >
inline float
vect< 2 >::cross(
	const vectr< 2, ANYCLASS_T >& v) const
{
	return get(0) * v[1] - get(1) * v[0];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect<3>
// specialization of vect for DIMENSION_T = 3
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class vect< 3 > : public vectr< 3, vect< 3 > >
{
public:

	vect()
	{}

	explicit vect(
		const float (&v)[3]);

	explicit vect(
		const float c0,
		const float c1,
		const float c2);

	////////////////////////////////////////////////////////////////////////////////////////////////
	template < class ANYCLASS_T >
	vect< 3 >& cross(													// cross-product of this and argument
		const vectr< 3, ANYCLASS_T >& v);

	template < class ANYCLASS0_T, class ANYCLASS1_T >
	vect< 3 >& cross(													// cross-product of arguments
		const vectr< 3, ANYCLASS0_T >& v0,
		const vectr< 3, ANYCLASS1_T >& v1);
};


inline vect< 3 >::vect(
	const float (&v)[3])
: vectr< 3, vect< 3 > >(v)
{}


inline vect< 3 >::vect(
	const float c0,
	const float c1,
	const float c2)
{
	set(0, c0);
	set(1, c1);
	set(2, c2);
}


template < class ANYCLASS_T >
inline vect< 3 >&
vect< 3 >::cross(
	const vectr< 3, ANYCLASS_T >& v)
{
	return *this = vect< 3 >(
		get(1) * v[2] - get(2) * v[1],
		get(2) * v[0] - get(0) * v[2],
		get(0) * v[1] - get(1) * v[0]);
}


template < class ANYCLASS0_T, class ANYCLASS1_T >
inline vect< 3 >&
vect< 3 >::cross(
	const vectr< 3, ANYCLASS0_T >& v0,
	const vectr< 3, ANYCLASS1_T >& v1)
{
	return *this = vect< 3 >(
		v0[1] * v1[2] - v0[2] * v1[1],
		v0[2] * v1[0] - v0[0] * v1[2],
		v0[0] * v1[1] - v0[1] * v1[0]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class vect<4>
// specialization of vect for DIMENSION_T = 4
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class vect< 4 > : public vectr< 4, vect< 4 > >
{
public:

	vect()
	{}

	explicit vect(
		const float (&v)[4]);

	explicit vect(
		const float c0,
		const float c1,
		const float c2,
		const float c3);
};


inline vect< 4 >::vect(
	const float (&v)[4])
: vectr< 4, vect< 4 > >(v)
{}


inline vect< 4 >::vect(
	const float c0,
	const float c1,
	const float c2,
	const float c3)
{
	set(0, c0);
	set(1, c1);
	set(2, c2);
	set(3, c3);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class quat
// quaternion abstraction, specialized for rotations, i.e. unit quaternion. expressed from an angle
// and an axis of rotation, q = cos(a/2) + i (x * sin(a/2)) + j (y * sin(a/2)) + k (z * sin(a/2))
// herein quaternions use an x, y, z, w vector layout, where the last component is the scalar part
////////////////////////////////////////////////////////////////////////////////////////////////////

class quat : public vectr< 4, quat >
{
public:

	quat()
	{}

	explicit quat(
		const float (&v)[4]);

	explicit quat(
		const float a,
		const vect< 3 >& axis);

	explicit quat(
		const float x,
		const float y,
		const float z,
		const float w);

	quat& qmur(						// quaternion product of this and argument
		const quat& q);

	quat& qmul(						// quaternion product of argument and this
		const quat& q);

	quat& qmul(						// quaternion product of arguments
		const quat& p,
		const quat& q);
};


inline quat::quat(
	const float (&v)[4])
: vectr< 4, quat >(v)
{}


inline quat::quat(
	const float a,
	const vect< 3 >& axis)
{
	const float sin_ha = sin(a * .5f);
	const float cos_ha = cos(a * .5f);

	vect< 3 >::castU(dekast()).mul(axis, sin_ha);

	set(3, cos_ha);
}


inline quat::quat(
	const float x,
	const float y,
	const float z,
	const float w)
{
	set(0, x);
	set(1, y);
	set(2, z);
	set(3, w);
}


inline quat&
quat::qmul(
	const quat& p,
	const quat& q)
{
	set(0, p[3] * q[0] + p[0] * q[3] + p[1] * q[2] - p[2] * q[1]);
	set(1, p[3] * q[1] - p[0] * q[2] + p[1] * q[3] + p[2] * q[0]);
	set(2, p[3] * q[2] + p[0] * q[1] - p[1] * q[0] + p[2] * q[3]);
	set(3, p[3] * q[3] - p[0] * q[0] - p[1] * q[1] - p[2] * q[2]);

	return *this;
}


inline quat&
quat::qmur(
	const quat& q)
{
	quat t;
	t.qmul(*this, q);

	return *this = t;
}


inline quat&
quat::qmul(
	const quat& q)
{
	quat t;
	t.qmul(q, *this);

	return *this = t;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class protomatx
// dimensionality-agnostic square matrix of reals
// warning: do not use directly, subclass like this:
//
//			class mymatx : public protomatx< myDimension, mymatx >
//
//			where mymatx does not introduce any new data members!
////////////////////////////////////////////////////////////////////////////////////////////////////

template < unsigned DIMENSION_T, class SUBCLASS_T >
class protomatx
{
	float m[DIMENSION_T][DIMENSION_T];

public:

	protomatx()
	{}

	////////////////////////////////////////////////////////////////////////////////////////////////
	static SUBCLASS_T& cast(
		float (&mat)[DIMENSION_T][DIMENSION_T]);

	static const SUBCLASS_T& cast(
		const float (&mat)[DIMENSION_T][DIMENSION_T]);

	static SUBCLASS_T& cast(
		float (&mat)[DIMENSION_T * DIMENSION_T]);

	static const SUBCLASS_T& cast(
		const float (&mat)[DIMENSION_T * DIMENSION_T]);

	////////////////////////////////////////////////////////////////////////////////////////////////
	typedef const float (& decast)[DIMENSION_T][DIMENSION_T];

	operator decast () const															// implicit typecast to 'const float[DIMENSION_T][DIMENSION_T]'
		{ return m; }

	float (& dekast())[DIMENSION_T][DIMENSION_T]										// explicit typecast to 'float[DIMENSION_T][DIMENSION_T]'
		{ return m; }

	const float (& decastF() const)[DIMENSION_T * DIMENSION_T]							// explicit typecast to 'const float[DIMENSION_T * DIMENSION_T]'
		{ return *reinterpret_cast< const float (*)[DIMENSION_T * DIMENSION_T] >(&m); }

	float (& dekastF())[DIMENSION_T * DIMENSION_T]										// explicit typecast to 'float[DIMENSION_T * DIMENSION_T]'
		{ return *reinterpret_cast< float (*)[DIMENSION_T * DIMENSION_T] >(&m); }

	////////////////////////////////////////////////////////////////////////////////////////////////
	operator SUBCLASS_T&();

	operator const SUBCLASS_T&() const;

	bool operator ==(
		const protomatx< DIMENSION_T, SUBCLASS_T >& mat) const;

	bool operator !=(
		const protomatx< DIMENSION_T, SUBCLASS_T >& mat) const;

	////////////////////////////////////////////////////////////////////////////////////////////////
	template < class ANYCLASS_T >
	SUBCLASS_T& set(																	// bulk mutator
		const protomatx< DIMENSION_T, ANYCLASS_T >& mat);

	SUBCLASS_T& set(																	// element mutator
		const unsigned i,
		const unsigned j,
		const float c);

	float get(																			// element accessor
		const unsigned i,
		const unsigned j) const;

	template < class ANYCLASS_T >
	ANYCLASS_T& getRow(																	// row accessor (copy)
		const unsigned i,
		vectr< DIMENSION_T, ANYCLASS_T >& v) const;

	template < class ANYCLASS_T >
	ANYCLASS_T& getCol(																	// col accessor (copy)
		const unsigned j,
		vectr< DIMENSION_T, ANYCLASS_T >& v) const;

	template < unsigned VECTDIM_T, class ANYCLASS_T >
	ANYCLASS_T& getRowS(																// sub-dimensional row accessor (copy)
		const unsigned i,
		vectr< VECTDIM_T, ANYCLASS_T >& v) const;

	template < unsigned VECTDIM_T, class ANYCLASS_T >
	ANYCLASS_T& getColS(																// sub-dimensional col accessor (copy)
		const unsigned j,
		vectr< VECTDIM_T, ANYCLASS_T >& v) const;

	template < class ANYCLASS_T >
	SUBCLASS_T& setRow(																	// row mutator
		const unsigned i,
		const vectr< DIMENSION_T, ANYCLASS_T >& v);

	template < class ANYCLASS_T >
	SUBCLASS_T& setCol(																	// col mutator
		const unsigned j,
		const vectr< DIMENSION_T, ANYCLASS_T >& v);

	template < unsigned VECTDIM_T, class ANYCLASS_T >
	SUBCLASS_T& setRowS(																// sub-dimensional row mutator
		const unsigned i,
		const vectr< VECTDIM_T, ANYCLASS_T >& v);

	template < unsigned VECTDIM_T, class ANYCLASS_T >
	SUBCLASS_T& setColS(																// sub-dimensional col mutator
		const unsigned j,
		const vectr< VECTDIM_T, ANYCLASS_T >& v);

	////////////////////////////////////////////////////////////////////////////////////////////////
	SUBCLASS_T& transpose();															// transpose this

	template < class ANYCLASS_T >
	SUBCLASS_T& transpose(																// transpose argument
		const protomatx< DIMENSION_T, ANYCLASS_T >& mat);

	template < class ANYCLASS_T >
	SUBCLASS_T& mur(																	// product of this and argument
		const protomatx< DIMENSION_T, ANYCLASS_T >& mat);

	template < class ANYCLASS_T >
	SUBCLASS_T& mul(																	// product of argument and this
		const protomatx< DIMENSION_T, ANYCLASS_T >& mat);

	template < class ANYCLASS0_T, class ANYCLASS1_T >
	SUBCLASS_T& mul(																	// product of arguments
		const protomatx< DIMENSION_T, ANYCLASS0_T >& mat0,
		const protomatx< DIMENSION_T, ANYCLASS1_T >& mat1);

	SUBCLASS_T& mul(																	// product of this and scalar argument
		const float c);

	template < class ANYCLASS_T >
	SUBCLASS_T& mul(																	// product of argument and scalar argument
		const protomatx< DIMENSION_T, ANYCLASS_T >& mat,
		const float c);

	SUBCLASS_T& mul(																	// element multiplicative mutator
		const unsigned i,
		const unsigned j,
		const float c);

	////////////////////////////////////////////////////////////////////////////////////////////////
	template < unsigned VECTDIM_T, class ANYCLASS0_T, class ANYCLASS1_T >
	void transform(																		// transform super-dimensional vector by this
		const vectr< VECTDIM_T, ANYCLASS0_T >& vi,
			  vectr< VECTDIM_T, ANYCLASS1_T >& vo) const;

	template < unsigned SUBDIM_T, unsigned VECTDIM_T, class ANYCLASS0_T, class ANYCLASS1_T >
	void transformS(																	// transform super-dimensional vector by leading submatrix of this
		const vectr< VECTDIM_T, ANYCLASS0_T >& vi,
			  vectr< VECTDIM_T, ANYCLASS1_T >& vo) const;

	template < unsigned VECTDIM_T, class ANYCLASS0_T, class ANYCLASS1_T >
	void transformT(																	// transform super-dimensional vector by (this)T
		const vectr< VECTDIM_T, ANYCLASS0_T >& vi,
			  vectr< VECTDIM_T, ANYCLASS1_T >& vo) const;

	template < unsigned SUBDIM_T, unsigned VECTDIM_T, class ANYCLASS0_T, class ANYCLASS1_T >
	void transformTS(																	// transform super-dimensional vector by leading submatrix of (this)T
		const vectr< VECTDIM_T, ANYCLASS0_T >& vi,
			  vectr< VECTDIM_T, ANYCLASS1_T >& vo) const;
};


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::cast(
	float (&mat)[DIMENSION_T][DIMENSION_T])
{
	return reinterpret_cast< SUBCLASS_T& >(mat);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline const SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::cast(
	const float (&mat)[DIMENSION_T][DIMENSION_T])
{
	return reinterpret_cast< const SUBCLASS_T& >(mat);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::cast(
	float (&mat)[DIMENSION_T * DIMENSION_T])
{
	return reinterpret_cast< SUBCLASS_T& >(mat);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline const SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::cast(
	const float (&mat)[DIMENSION_T * DIMENSION_T])
{
	return reinterpret_cast< const SUBCLASS_T& >(mat);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline protomatx< DIMENSION_T, SUBCLASS_T >::operator SUBCLASS_T&()
{
	return *static_cast< SUBCLASS_T* >(this);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline protomatx< DIMENSION_T, SUBCLASS_T >::operator const SUBCLASS_T&() const
{
	return *static_cast< const SUBCLASS_T* >(this);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline bool
protomatx< DIMENSION_T, SUBCLASS_T >::operator ==(
	const protomatx< DIMENSION_T, SUBCLASS_T >& mat) const
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		for (unsigned j = 0; j < DIMENSION_T; ++j)
			if (m[i][j] != mat.m[i][j])
				return false;

	return true;
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline bool
protomatx< DIMENSION_T, SUBCLASS_T >::operator !=(
	const protomatx< DIMENSION_T, SUBCLASS_T >& mat) const
{
	return !operator ==(mat);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::set(
	const protomatx< DIMENSION_T, ANYCLASS_T >& mat)
{
	return *this = cast(mat);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::set(
	const unsigned i,
	const unsigned j,
	const float c)
{
	assert(i < DIMENSION_T && j < DIMENSION_T);

	m[i][j] = c;

	return *this;
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline float
protomatx< DIMENSION_T, SUBCLASS_T >::get(
	const unsigned i,
	const unsigned j) const
{
	assert(i < DIMENSION_T && j < DIMENSION_T);

	return m[i][j];
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < unsigned VECTDIM_T, class ANYCLASS_T >
inline ANYCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::getRowS(
	const unsigned i,
	vectr< VECTDIM_T, ANYCLASS_T >& v) const
{
	assert(VECTDIM_T <= DIMENSION_T);
	assert(i < DIMENSION_T);

	sparse_copy< VECTDIM_T, 1, 1 >(v.dekast(), m[i]);

	return v;
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < unsigned VECTDIM_T, class ANYCLASS_T >
inline ANYCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::getColS(
	const unsigned j,
	vectr< VECTDIM_T, ANYCLASS_T >& v) const
{
	assert(VECTDIM_T <= DIMENSION_T);
	assert(j < DIMENSION_T);

	sparse_copy< VECTDIM_T, 1, DIMENSION_T >(v.dekast(), m[0] + j);

	return v;
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline ANYCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::getRow(
	const unsigned i,
	vectr< DIMENSION_T, ANYCLASS_T >& v) const
{
	return getRowS< DIMENSION_T, ANYCLASS_T >(i, v);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline ANYCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::getCol(
	const unsigned j,
	vectr< DIMENSION_T, ANYCLASS_T > &v) const
{
	return getColS< DIMENSION_T, ANYCLASS_T >(j, v);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < unsigned VECTDIM_T, class ANYCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::setRowS(
	const unsigned i,
	const vectr< VECTDIM_T, ANYCLASS_T >& v)
{
	assert(VECTDIM_T <= DIMENSION_T);
	assert(i < DIMENSION_T);

	sparse_copy< VECTDIM_T, 1, 1, float >(m[i], v);

	return *this;
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < unsigned VECTDIM_T, class ANYCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::setColS(
	const unsigned j,
	const vectr< VECTDIM_T, ANYCLASS_T >& v)
{
	assert(VECTDIM_T <= DIMENSION_T);
	assert(j < DIMENSION_T);

	sparse_copy< VECTDIM_T, DIMENSION_T, 1, float >(m[0] + j, v);

	return *this;
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::setRow(
	const unsigned i,
	const vectr< DIMENSION_T, ANYCLASS_T >& v)
{
	return setRowS< DIMENSION_T, ANYCLASS_T >(i, v);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::setCol(
	const unsigned j,
	const vectr< DIMENSION_T, ANYCLASS_T >& v)
{
	return setColS< DIMENSION_T, ANYCLASS_T >(j, v);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::transpose(
	const protomatx< DIMENSION_T, ANYCLASS_T >& mat)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		for (unsigned j = 0; j < DIMENSION_T; ++j)
			m[j][i] = mat[i][j];

	return *this;
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::transpose()
{
	const protomatx< DIMENSION_T, SUBCLASS_T > t(*this);

	return transpose(t);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS0_T, class ANYCLASS1_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::mul(
	const protomatx< DIMENSION_T, ANYCLASS0_T >& mat0,
	const protomatx< DIMENSION_T, ANYCLASS1_T >& mat1)
{
#if	defined(REND_MATX_MUL_V2__)

	for (unsigned i = 0; i < DIMENSION_T; ++i)
	{
		for (unsigned j = 0; j < DIMENSION_T; ++j)
			m[i][j] = mat0[i][0] * mat1[0][j];

		for (unsigned j = 1; j < DIMENSION_T; ++j)
			for (unsigned k = 0; k < DIMENSION_T; ++k)
				m[i][k] += mat0[i][j] * mat1[j][k];
	}

#else

	for (unsigned i = 0; i < DIMENSION_T; ++i)
		for (unsigned j = 0; j < DIMENSION_T; ++j)
			m[i][j] = rend::dot< DIMENSION_T, 1, DIMENSION_T >(mat0[i], mat1[0] + j);

#endif

	return *this;
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::mur(
	const protomatx< DIMENSION_T, ANYCLASS_T >& mat)
{
	const protomatx< DIMENSION_T, SUBCLASS_T > t(*this);

	return mul(t, mat);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::mul(
	const protomatx< DIMENSION_T, ANYCLASS_T >& mat)
{
	const protomatx< DIMENSION_T, SUBCLASS_T > t(*this);

	return mul(mat, t);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::mul(
	const float c)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		for (unsigned j = 0; j < DIMENSION_T; ++j)
			m[i][j] *= c;

	return *this;
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::mul(
	const protomatx< DIMENSION_T, ANYCLASS_T >& mat,
	const float c)
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		for (unsigned j = 0; j < DIMENSION_T; ++j)
			m[i][j] = mat[i][j] * c;

	return *this;
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
inline SUBCLASS_T&
protomatx< DIMENSION_T, SUBCLASS_T >::mul(
	const unsigned i,
	const unsigned j,
	const float c)
{
	m[i][j] *= c;

	return *this;
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < unsigned SUBDIM_T, unsigned VECTDIM_T, class ANYCLASS0_T, class ANYCLASS1_T >
inline void
protomatx< DIMENSION_T, SUBCLASS_T >::transformS(
	const vectr< VECTDIM_T, ANYCLASS0_T >& vi,
		  vectr< VECTDIM_T, ANYCLASS1_T >& vo) const
{
	assert(SUBDIM_T <= DIMENSION_T && SUBDIM_T <= VECTDIM_T);

	unsigned i;

	for (i = 0; i < SUBDIM_T; ++i)
		vo.set(i, rend::dot< SUBDIM_T, 1, 1 >(m[i], vi));

	for (; i < VECTDIM_T; ++i)
		vo.set(i, vi[i]);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < unsigned VECTDIM_T, class ANYCLASS0_T, class ANYCLASS1_T >
inline void
protomatx< DIMENSION_T, SUBCLASS_T >::transform(
	const vectr< VECTDIM_T, ANYCLASS0_T >& vi,
		  vectr< VECTDIM_T, ANYCLASS1_T >& vo) const
{
	transformS< DIMENSION_T >(vi, vo);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < unsigned SUBDIM_T, unsigned VECTDIM_T, class ANYCLASS0_T, class ANYCLASS1_T >
inline void
protomatx< DIMENSION_T, SUBCLASS_T >::transformTS(
	const vectr< VECTDIM_T, ANYCLASS0_T >& vi,
		  vectr< VECTDIM_T, ANYCLASS1_T >& vo) const
{
	assert(SUBDIM_T <= DIMENSION_T && SUBDIM_T <= VECTDIM_T);

	unsigned i;

	for (i = 0; i < SUBDIM_T; ++i)
		vo.set(i, rend::dot< SUBDIM_T, 1, DIMENSION_T >(vi, m[0] + i));

	for (; i < VECTDIM_T; ++i)
		vo.set(i, vi[i]);
}


template < unsigned DIMENSION_T, class SUBCLASS_T >
template < unsigned VECTDIM_T, class ANYCLASS0_T, class ANYCLASS1_T >
inline void
protomatx< DIMENSION_T, SUBCLASS_T >::transformT(
	const vectr< VECTDIM_T, ANYCLASS0_T >& vi,
		  vectr< VECTDIM_T, ANYCLASS1_T >& vo) const
{
	transformTS< DIMENSION_T >(vi, vo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class matx
// instantiatable, dimensionality-agnostic square matrix of reals
////////////////////////////////////////////////////////////////////////////////////////////////////

template < unsigned DIMENSION_T >
class matx : public protomatx< DIMENSION_T, matx< DIMENSION_T > >
{
public:

	matx()
	{}

	explicit matx(
		const float (&mat)[DIMENSION_T][DIMENSION_T]);
	explicit matx(
		const float (&mat)[DIMENSION_T * DIMENSION_T]);
};


template < unsigned DIMENSION_T >
inline matx< DIMENSION_T >::matx(
	const float (&mat)[DIMENSION_T][DIMENSION_T])
{
	*this = cast(mat);
}


template < unsigned DIMENSION_T >
inline matx< DIMENSION_T >::matx(
	const float (&mat)[DIMENSION_T * DIMENSION_T])
{
	*this = cast(mat);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class matx<3>
// specialization of matx for DIMENSION_T = 3
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class matx< 3 > : public protomatx< 3, matx< 3 > >
{
public:

	matx()
	{}

	explicit matx(
		const float (&mat)[3][3]);
	explicit matx(
		const float (&mat)[3 * 3]);

	explicit matx(
		const float c00, const float c01, const float c02,
		const float c10, const float c11, const float c12,
		const float c20, const float c21, const float c22);

	explicit matx(
		const quat& q);

	////////////////////////////////////////////////////////////////////////////////////////////////
	matx< 3 >& identity();																	// identity mutator

	matx< 3 >& rotate(																		// rotation mutator
		const float a,
		const float x,
		const float y,
		const float z);

	matx< 3 >& scale(																		// scale mutator
		const float x,
		const float y,
		const float z);
};


inline matx< 3 >::matx(
	const float (&mat)[3][3])
{
	*this = cast(mat);
}


inline matx< 3 >::matx(
	const float (&mat)[3 * 3])
{
	*this = cast(mat);
}


inline matx< 3 >::matx(
	const float c00, const float c01, const float c02,
	const float c10, const float c11, const float c12,
	const float c20, const float c21, const float c22)
{
	set(0, 0, c00);
	set(0, 1, c01);
	set(0, 2, c02);

	set(1, 0, c10);
	set(1, 1, c11);
	set(1, 2, c12);

	set(2, 0, c20);
	set(2, 1, c21);
	set(2, 2, c22);
}


inline matx< 3 >::matx(
	const quat& q)
{
	*this = matx< 3 >(
		1.f - 2.f * (q[1] * q[1] + q[2] * q[2]), 2.f * (q[0] * q[1] - q[2] * q[3]), 2.f * (q[0] * q[2] + q[1] * q[3]),
		2.f * (q[0] * q[1] + q[2] * q[3]), 1.f - 2.f * (q[0] * q[0] + q[2] * q[2]), 2.f * (q[1] * q[2] - q[0] * q[3]),
		2.f * (q[0] * q[2] - q[1] * q[3]), 2.f * (q[1] * q[2] + q[0] * q[3]), 1.f - 2.f * (q[0] * q[0] + q[1] * q[1]));
}


inline matx< 3 >& matx< 3 >::identity()
{
	return *this = matx< 3 >(
		1,  0,  0,
		0,  1,  0,
		0,  0,  1);
}

// rotate()	: produces a right-hand rotation transform from an angle and a rotation axis
//		- a,		const float			: rotation angle (radians),				input
//		- x,		const float			: rotation axis' x-component,			input
//		- y,		const float			: rotation axis' y-component,			input
//		- z,		const float			: rotation axis' z-component,			input
// returns
//		matx< 3 >&
// note
//		- rotation axis must be a unit vector

inline matx< 3 >& matx< 3 >::rotate(
	const float a,
	const float x,
	const float y,
	const float z)
{
	const float sin_a = sin(a);
	const float cos_a = cos(a);

	return *this = matx< 3 >(
		x * x + cos_a * (1 - x * x),		 y * x - cos_a * (y * x) - sin_a * z, z * x - cos_a * (z * x) + sin_a * y,
		x * y - cos_a * (x * y) + sin_a * z, y * y + cos_a * (1 - y * y),		  z * y - cos_a * (z * y) - sin_a * x,
		x * z - cos_a * (x * z) - sin_a * y, y * z - cos_a * (y * z) + sin_a * x, z * z + cos_a * (1 - z * z));
}


inline matx< 3 >& matx< 3 >::scale(
	const float x,
	const float y,
	const float z)
{
	return *this = matx< 3 >(
		x,  0,  0,
		0,  y,  0,
		0,  0,  z);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class matx<4>
// specialization of matx for DIMENSION_T = 4
////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
class matx< 4 > : public protomatx< 4, matx< 4 > >
{
public:

	matx()
	{}

	explicit matx(
		const float (&mat)[4][4]);
	explicit matx(
		const float (&mat)[4 * 4]);

	explicit matx(
		const float c00, const float c01, const float c02, const float c03,
		const float c10, const float c11, const float c12, const float c13,
		const float c20, const float c21, const float c22, const float c23,
		const float c30, const float c31, const float c32, const float c33);

	explicit matx(
		const quat& q);

	////////////////////////////////////////////////////////////////////////////////////////////////
	matx< 4 >& identity();																	// identity mutator

	matx< 4 >& rotate(																		// rotation mutator
		const float a,
		const float x,
		const float y,
		const float z);

	matx< 4 >& translate(																	// translation mutator
		const float x,
		const float y,
		const float z);

	matx< 4 >& scale(																		// scale mutator
		const float x,
		const float y,
		const float z);

	matx< 4 >& persp(																		// persp projection mutator
		const float l, const float r,
		const float b, const float t,
		const float n, const float f);

	matx< 4 >& ortho(																		// ortho projection mutator
		const float l, const float r,
		const float b, const float t,
		const float n, const float f);

	////////////////////////////////////////////////////////////////////////////////////////////////
	bool invert();																			// invert this

	template < class ANYCLASS_T >
	bool invert(																			// invert argument
		const protomatx< 4, ANYCLASS_T >& mat);

	template < class ANYCLASS_T >
	matx< 4 >& invert_unsafe(																// invert argument
		const protomatx< 4, ANYCLASS_T >& mat);

	////////////////////////////////////////////////////////////////////////////////////////////////
	template < unsigned VECTDIM_T, class ANYCLASS0_T, class ANYCLASS1_T >
	void transform3(
		const vectr< VECTDIM_T, ANYCLASS0_T >& vi,											// transform a non-homogeneous super-dimensional vector
			  vectr< VECTDIM_T, ANYCLASS1_T >& vo) const;									// by this, applying translation as if w = 1
};


inline matx< 4 >::matx(
	const float (&mat)[4][4])
{
	*this = cast(mat);
}


inline matx< 4 >::matx(
	const float (&mat)[4 * 4])
{
	*this = cast(mat);
}


inline matx< 4 >::matx(
	const float c00, const float c01, const float c02, const float c03,
	const float c10, const float c11, const float c12, const float c13,
	const float c20, const float c21, const float c22, const float c23,
	const float c30, const float c31, const float c32, const float c33)
{
	set(0, 0, c00);
	set(0, 1, c01);
	set(0, 2, c02);
	set(0, 3, c03);

	set(1, 0, c10);
	set(1, 1, c11);
	set(1, 2, c12);
	set(1, 3, c13);

	set(2, 0, c20);
	set(2, 1, c21);
	set(2, 2, c22);
	set(2, 3, c23);

	set(3, 0, c30);
	set(3, 1, c31);
	set(3, 2, c32);
	set(3, 3, c33);
}


inline matx< 4 >::matx(
	const quat& q)
{
	*this = matx< 4 >(
		1.f - 2.f * (q[1] * q[1] + q[2] * q[2]), 2.f * (q[0] * q[1] - q[2] * q[3]), 2.f * (q[0] * q[2] + q[1] * q[3]), 0.f,
		2.f * (q[0] * q[1] + q[2] * q[3]), 1.f - 2.f * (q[0] * q[0] + q[2] * q[2]), 2.f * (q[1] * q[2] - q[0] * q[3]), 0.f,
		2.f * (q[0] * q[2] - q[1] * q[3]), 2.f * (q[1] * q[2] + q[0] * q[3]), 1.f - 2.f * (q[0] * q[0] + q[1] * q[1]), 0.f,
		0.f, 0.f, 0.f, 1.f);
}


inline matx< 4 >& matx< 4 >::identity()
{
	return *this = matx< 4 >(
		1,  0,  0,  0,
		0,  1,  0,  0,
		0,  0,  1,  0,
		0,  0,  0,  1);
}

// rotate()	: produces a right-hand rotation transform from an angle and a rotation axis
//		- a,		const float			: rotation angle (radians),				input
//		- x,		const float			: rotation axis' x-component,			input
//		- y,		const float			: rotation axis' y-component,			input
//		- z,		const float			: rotation axis' z-component,			input
// returns
//		matx< 4 >&
// note
//		- rotation axis must be a unit vector

inline matx< 4 >& matx< 4 >::rotate(
	const float a,
	const float x,
	const float y,
	const float z)
{
	const float sin_a = sin(a);
	const float cos_a = cos(a);

	return *this = matx< 4 >(
		x * x + cos_a * (1 - x * x),		 y * x - cos_a * (y * x) - sin_a * z, z * x - cos_a * (z * x) + sin_a * y,	0,
		x * y - cos_a * (x * y) + sin_a * z, y * y + cos_a * (1 - y * y),		  z * y - cos_a * (z * y) - sin_a * x,	0,
		x * z - cos_a * (x * z) - sin_a * y, y * z - cos_a * (y * z) + sin_a * x, z * z + cos_a * (1 - z * z),			0,
		0,									 0,									  0,									1);
}


inline matx< 4 >& matx< 4 >::translate(
	const float x,
	const float y,
	const float z)
{
	return *this = matx< 4 >(
		1,  0,  0,  x,
		0,  1,  0,  y,
		0,  0,  1,  z,
		0,  0,  0,  1);
}


inline matx< 4 >& matx< 4 >::scale(
	const float x,
	const float y,
	const float z)
{
	return *this = matx< 4 >(
		x,  0,  0,  0,
		0,  y,  0,  0,
		0,  0,  z,  0,
		0,  0,  0,  1);
}

// persp()	: builds a perspective-projection transform
//		- l,		const float		: left edge of viewport's mapping onto the near clipping plane,		input
//		- r,		const float		: right edge of viewport's mapping onto the near clipping plane,	input
//		- b,		const float		: bottom edge of viewport's mapping onto the near clipping plane,	input
//		- t,		const float		: top edge of viewport's mapping onto the near clipping plane,		input
//		- n,		const float		: distance (along the view vector) to the near clipping plane,		input
//		- f,		const float		: distance (along the view vector) to the far clipping plane,		input
// returns
//		matx< 4 >&
// note
//		- camera space taken as right-handed, positive y-axis pointing up
//		  and negative z-axis pointing in the direction of the view vector
//		- the distances to the near and far clipping planes are in absolute values,
//		  whereas the two planes are at -n and -f units from the origin, respectively
//		- clipping is	-Wc <= Xc <= Wc,	and NDC is	-1 <= Xd <= 1
//						-Wc <= Yc <= Wc					-1 <= Yd <= 1
//						-Wc <= Zc <= Wc					-1 <= Zd <= 1

inline matx< 4 >& matx< 4 >::persp(
	const float l, const float r,
	const float b, const float t,
	const float n, const float f)
{
	return *this = matx< 4 >(
		2 * n / (r - l), 0,					(r + l) / (r - l), 0,
		0,				 2 * n / (t - b),	(t + b) / (t - b), 0,
		0,				 0,				  - (f + n) / (f - n), - 2 * f * n / (f - n),
		0,				 0,				  - 1,				   0);
}


// ortho()	: builds a parallel-projection transform
//		- l,		const float		: left edge of viewport's mapping onto the near clipping plane,		input
//		- r,		const float		: right edge of viewport's mapping onto the near clipping plane,	input
//		- b,		const float		: bottom edge of viewport's mapping onto the near clipping plane,	input
//		- t,		const float		: top edge of viewport's mapping onto the near clipping plane,		input
//		- n,		const float		: distance (along the view vector) to the near clipping plane,		input
//		- f,		const float		: distance (along the view vector) to the far clipping plane,		input
// returns
//		matx< 4 >&
// note
//		- camera space taken as right-handed, positive y-axis pointing up
//		  and negative z-axis pointing in the direction of the view vector
//		- the distances to the near and far clipping planes are in absolute values,
//		  whereas the two planes are at -n and -f units from the origin, respectively
//		- clipping is	-Wc <= Xc <= Wc,	and NDC is	-1 <= Xd <= 1
//						-Wc <= Yc <= Wc					-1 <= Yd <= 1
//						-Wc <= Zc <= Wc					-1 <= Zd <= 1

inline matx< 4 >& matx< 4 >::ortho(
	const float l, const float r,
	const float b, const float t,
	const float n, const float f)
{
	return *this = matx< 4 >(
		2 / (r - l), 0,			  0,			 - (r + l) / (r - l),
		0,			 2 / (t - b), 0,			 - (t + b) / (t - b),
		0,			 0,			  - 2 / (f - n), - (f + n) / (f - n),
		0,			 0,			  0,			 1);
}


// matx4::invert()	: computes the inverse of the given matrix if it is invertible
//		- mat,		const float (&)[4][4]	: argument matrix,		input
// returns
//		bool		: true - success, false - matrix is non-invertible
// note
//		- uses Cramer's rule for matrix inversion
//		- based on original code by Dinko Tenev

template < class ANYCLASS_T >
bool matx< 4 >::invert(
	const protomatx< 4, ANYCLASS_T >& mat)
{
	float dst[16], src[16], tmp[12];

	// transpose source matrix
	cast(src).transpose(mat);

	// calculate pairs for first 8 elements (cofactors)
	tmp[ 0]  = src[10] * src[15];
	tmp[ 1]  = src[11] * src[14];
	tmp[ 2]  = src[ 9] * src[15];
	tmp[ 3]  = src[11] * src[13];
	tmp[ 4]  = src[ 9] * src[14];
	tmp[ 5]  = src[10] * src[13];
	tmp[ 6]  = src[ 8] * src[15];
	tmp[ 7]  = src[11] * src[12];
	tmp[ 8]  = src[ 8] * src[14];
	tmp[ 9]  = src[10] * src[12];
	tmp[10]  = src[ 8] * src[13];
	tmp[11]  = src[ 9] * src[12];

	// calculate first 8 elements (cofactors)
	dst[ 0]  = tmp[ 0] * src[ 5] + tmp[ 3] * src[ 6] + tmp[ 4] * src[ 7];
	dst[ 0] -= tmp[ 1] * src[ 5] + tmp[ 2] * src[ 6] + tmp[ 5] * src[ 7];
	dst[ 1]  = tmp[ 1] * src[ 4] + tmp[ 6] * src[ 6] + tmp[ 9] * src[ 7];
	dst[ 1] -= tmp[ 0] * src[ 4] + tmp[ 7] * src[ 6] + tmp[ 8] * src[ 7];
	dst[ 2]  = tmp[ 2] * src[ 4] + tmp[ 7] * src[ 5] + tmp[10] * src[ 7];
	dst[ 2] -= tmp[ 3] * src[ 4] + tmp[ 6] * src[ 5] + tmp[11] * src[ 7];
	dst[ 3]  = tmp[ 5] * src[ 4] + tmp[ 8] * src[ 5] + tmp[11] * src[ 6];
	dst[ 3] -= tmp[ 4] * src[ 4] + tmp[ 9] * src[ 5] + tmp[10] * src[ 6];
	dst[ 4]  = tmp[ 1] * src[ 1] + tmp[ 2] * src[ 2] + tmp[ 5] * src[ 3];
	dst[ 4] -= tmp[ 0] * src[ 1] + tmp[ 3] * src[ 2] + tmp[ 4] * src[ 3];
	dst[ 5]  = tmp[ 0] * src[ 0] + tmp[ 7] * src[ 2] + tmp[ 8] * src[ 3];
	dst[ 5] -= tmp[ 1] * src[ 0] + tmp[ 6] * src[ 2] + tmp[ 9] * src[ 3];
	dst[ 6]  = tmp[ 3] * src[ 0] + tmp[ 6] * src[ 1] + tmp[11] * src[ 3];
	dst[ 6] -= tmp[ 2] * src[ 0] + tmp[ 7] * src[ 1] + tmp[10] * src[ 3];
	dst[ 7]  = tmp[ 4] * src[ 0] + tmp[ 9] * src[ 1] + tmp[10] * src[ 2];
	dst[ 7] -= tmp[ 5] * src[ 0] + tmp[ 8] * src[ 1] + tmp[11] * src[ 2];

	// calculate pairs for second 8 elements (cofactors)
	tmp[ 0]  = src[ 2] * src[ 7];
	tmp[ 1]  = src[ 3] * src[ 6];
	tmp[ 2]  = src[ 1] * src[ 7];
	tmp[ 3]  = src[ 3] * src[ 5];
	tmp[ 4]  = src[ 1] * src[ 6];
	tmp[ 5]  = src[ 2] * src[ 5];
	tmp[ 6]  = src[ 0] * src[ 7];
	tmp[ 7]  = src[ 3] * src[ 4];
	tmp[ 8]  = src[ 0] * src[ 6];
	tmp[ 9]  = src[ 2] * src[ 4];
	tmp[10]  = src[ 0] * src[ 5];
	tmp[11]  = src[ 1] * src[ 4];

	// calculate second 8 elements (cofactors)
	dst[ 8]  = tmp[ 0] * src[13] + tmp[ 3] * src[14] + tmp[ 4] * src[15];
	dst[ 8] -= tmp[ 1] * src[13] + tmp[ 2] * src[14] + tmp[ 5] * src[15];
	dst[ 9]  = tmp[ 1] * src[12] + tmp[ 6] * src[14] + tmp[ 9] * src[15];
	dst[ 9] -= tmp[ 0] * src[12] + tmp[ 7] * src[14] + tmp[ 8] * src[15];
	dst[10]  = tmp[ 2] * src[12] + tmp[ 7] * src[13] + tmp[10] * src[15];
	dst[10] -= tmp[ 3] * src[12] + tmp[ 6] * src[13] + tmp[11] * src[15];
	dst[11]  = tmp[ 5] * src[12] + tmp[ 8] * src[13] + tmp[11] * src[14];
	dst[11] -= tmp[ 4] * src[12] + tmp[ 9] * src[13] + tmp[10] * src[14];
	dst[12]  = tmp[ 2] * src[10] + tmp[ 5] * src[11] + tmp[ 1] * src[ 9];
	dst[12] -= tmp[ 4] * src[11] + tmp[ 0] * src[ 9] + tmp[ 3] * src[10];
	dst[13]  = tmp[ 8] * src[11] + tmp[ 0] * src[ 8] + tmp[ 7] * src[10];
	dst[13] -= tmp[ 6] * src[10] + tmp[ 9] * src[11] + tmp[ 1] * src[ 8];
	dst[14]  = tmp[ 6] * src[ 9] + tmp[11] * src[11] + tmp[ 3] * src[ 8];
	dst[14] -= tmp[10] * src[11] + tmp[ 2] * src[ 8] + tmp[ 7] * src[ 9];
	dst[15]  = tmp[10] * src[10] + tmp[ 4] * src[ 8] + tmp[ 9] * src[ 9];
	dst[15] -= tmp[ 8] * src[ 9] + tmp[11] * src[10] + tmp[ 5] * src[ 8];

	// calculate the reciprocal determinant and obtain the inverse
	const float eps = (float) 1e-15;
	const float det = src[0] * dst[0] + src[1] * dst[1] + src[2] * dst[2] + src[3] * dst[3];

	if (fabs(det) < eps)
		return false;

	mul(cast(dst), 1.f / det);

	return true;
}


// matx4::invert()	: computes the inverse of this if it is invertible
// returns
//		bool		: true - success, false - matrix is non-invertible

inline bool matx< 4 >::invert()
{
	return invert(*this);
}


// matx4::invert_unsafe()	: computes the inverse of this as if matrix was invertible
// note
//		- use at own risk

template < class ANYCLASS_T >
inline matx< 4 >&
matx< 4 >::invert_unsafe(
	const protomatx< 4, ANYCLASS_T >& mat)
{
	const bool invertible = invert(mat);
	assert(invertible);
	(void) invertible;

	return *this;
}


template < unsigned VECTDIM_T, class ANYCLASS0_T, class ANYCLASS1_T >
void matx< 4 >::transform3(
	const vectr< VECTDIM_T, ANYCLASS0_T >& vi,
		  vectr< VECTDIM_T, ANYCLASS1_T >& vo) const
{
	transformS< 3 >(vi, vo);

	vect< 3 >& vo3 = vect< 3 >::castU(vo.dekast());
	vo3.add(vect< 3 >(
		get(0, 3),
		get(1, 3),
		get(2, 3)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class protorect
// type- and dimensionality-agnostic axial slice (line segment in 1D, rectangle in 2D, box in 3D)
// warning: do not use directly, subclass like this:
//
//			class myrect : public protorect< myScalarType, myDimension, myrect >
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
class protorect
{
protected:
	SCALTYPE_T m_min[DIMENSION_T];
	SCALTYPE_T m_max[DIMENSION_T];

	protorect() {}

public:
	template < class ANYCLASS0_T, class ANYCLASS1_T >
	protorect(
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS0_T >& v0,
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS1_T >& v1)
	{
		for (unsigned i = 0; i < DIMENSION_T; ++i)
		{
			m_min[i] = v0[i] < v1[i] ? v0[i] : v1[i];
			m_max[i] = v0[i] < v1[i] ? v1[i] : v0[i];
		}
	}

	enum flag_direct {};

	template < class ANYCLASS0_T, class ANYCLASS1_T >
	protorect(
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS0_T >& v0,
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS1_T >& v1,
		const typename protorect::flag_direct&)
	{
		protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS0_T >::cast(m_min) = v0;
		protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS1_T >::cast(m_max) = v1;
	}

	bool is_valid() const;

	const SCALTYPE_T (& get_min() const)[DIMENSION_T]
	{
		return m_min;
	}

	const SCALTYPE_T (& get_max() const)[DIMENSION_T]
	{
		return m_max;
	}

	SCALTYPE_T get_min(
		const unsigned i) const
	{
		assert(i < DIMENSION_T);
		return m_min[i];
	}

	SCALTYPE_T get_max(
		const unsigned i) const
	{
		assert(i < DIMENSION_T);
		return m_max[i];
	}

	template < class ANYCLASS_T >
	bool has_overlap_open(
		const protorect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& oth) const;

	template < class ANYCLASS_T >
	bool has_overlap_closed(
		const protorect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& oth) const;

	template < class ANYCLASS_T >
	bool contains_open(
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v) const;

	template < class ANYCLASS_T >
	bool contains_closed(
		const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v) const;

	template < class ANYCLASS_T >
	bool contains_open(
		const protorect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& oth) const;

	template < class ANYCLASS_T >
	bool contains_closed(
		const protorect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& oth) const;
};


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
inline bool
protorect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::is_valid() const
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m_min[i] > m_max[i])
			return false;

	return true;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline bool
protorect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::has_overlap_open(
	const protorect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& oth) const
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m_min[i] >= oth.get_max(i))
			return false;

	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m_max[i] <= oth.get_min(i))
			return false;

	return true;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline bool
protorect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::has_overlap_closed(
	const protorect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& oth) const
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m_min[i] > oth.get_max(i))
			return false;

	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m_max[i] < oth.get_min(i))
			return false;

	return true;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline bool
protorect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::contains_open(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v) const
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m_min[i] >= v.get(i))
			return false;

	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m_max[i] <= v.get(i))
			return false;

	return true;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline bool
protorect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::contains_closed(
	const protovect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& v) const
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m_min[i] > v.get(i))
			return false;

	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m_max[i] < v.get(i))
			return false;

	return true;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline bool
protorect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::contains_open(
	const protorect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& oth) const
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m_min[i] >= oth.get_min(i))
			return false;

	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m_max[i] <= oth.get_max(i))
			return false;

	return true;
}


template < typename SCALTYPE_T, unsigned DIMENSION_T, class SUBCLASS_T >
template < class ANYCLASS_T >
inline bool
protorect< SCALTYPE_T, DIMENSION_T, SUBCLASS_T >::contains_closed(
	const protorect< SCALTYPE_T, DIMENSION_T, ANYCLASS_T >& oth) const
{
	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m_min[i] > oth.get_min(i))
			return false;

	for (unsigned i = 0; i < DIMENSION_T; ++i)
		if (m_max[i] < oth.get_max(i))
			return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class rect2
// type-agnostic rectangle
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename SCALTYPE_T >
class rect2 : public protorect< SCALTYPE_T, 2, rect2< SCALTYPE_T > >
{
protected:
	rect2() {}

public:
	enum { dimension = 2 };

	template < class ANYCLASS0_T, class ANYCLASS1_T >
	rect2(
		const protovect< SCALTYPE_T, dimension, ANYCLASS0_T >& v0,
		const protovect< SCALTYPE_T, dimension, ANYCLASS1_T >& v1)
	: protorect< SCALTYPE_T, dimension, rect2 >(v0, v1)
	{}

	template < class ANYCLASS0_T, class ANYCLASS1_T >
	rect2(
		const protovect< SCALTYPE_T, dimension, ANYCLASS0_T >& v0,
		const protovect< SCALTYPE_T, dimension, ANYCLASS1_T >& v1,
		const typename rect2::flag_direct& f)
	: protorect< SCALTYPE_T, dimension, rect2 >(v0, v1, f)
	{}

	rect2(
		const SCALTYPE_T& x0,
		const SCALTYPE_T& y0,
		const SCALTYPE_T& x1,
		const SCALTYPE_T& y1)
	{
		this->m_min[0] = x0 < x1 ? x0 : x1;
		this->m_min[1] = y0 < y1 ? y0 : y1;
		this->m_max[0] = x0 < x1 ? x1 : x0;
		this->m_max[1] = y0 < y1 ? y1 : y0;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// class rect3
// type-agnostic box
////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename SCALTYPE_T >
class rect3 : public protorect< SCALTYPE_T, 3, rect3< SCALTYPE_T > >
{
protected:
	rect3() {}

public:
	enum { dimension = 3 };

	template < class ANYCLASS0_T, class ANYCLASS1_T >
	rect3(
		const protovect< SCALTYPE_T, dimension, ANYCLASS0_T >& v0,
		const protovect< SCALTYPE_T, dimension, ANYCLASS1_T >& v1)
	: protorect< SCALTYPE_T, dimension, rect3 >(v0, v1)
	{}

	template < class ANYCLASS0_T, class ANYCLASS1_T >
	rect3(
		const protovect< SCALTYPE_T, dimension, ANYCLASS0_T >& v0,
		const protovect< SCALTYPE_T, dimension, ANYCLASS1_T >& v1,
		const typename rect3::flag_direct& f)
	: protorect< SCALTYPE_T, dimension, rect3 >(v0, v1, f)
	{}

	rect3(
		const SCALTYPE_T& x0,
		const SCALTYPE_T& y0,
		const SCALTYPE_T& z0,
		const SCALTYPE_T& x1,
		const SCALTYPE_T& y1,
		const SCALTYPE_T& z1)
	{
		this->m_min[0] = x0 < x1 ? x0 : x1;
		this->m_min[1] = y0 < y1 ? y0 : y1;
		this->m_min[2] = z0 < z1 ? z0 : z1;
		this->m_max[0] = x0 < x1 ? x1 : x0;
		this->m_max[1] = y0 < y1 ? y1 : y0;
		this->m_max[2] = z0 < z1 ? z1 : z0;
	}
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// utility functions and definitions
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef ivect< 2 >	ivect2;
typedef ivect< 3 >	ivect3;
typedef ivect< 4 >	ivect4;

typedef vect< 2 >	vect2;
typedef vect< 3 >	vect3;
typedef vect< 4 >	vect4;

typedef matx< 3 >	matx3;
typedef matx< 4 >	matx4;

// clip_by_plane()	: clips a polygon by a plane, 'inside' being the halfspace of positive distances
//		DIMENSION_T									: must be of R3 or greater
//		- plane,	const float (&)[4]				: halfspaces border,						input
//		- poly,		const float (**)[DIMENSION_T]	: array containing a polygon definition,	input/output
//		- start,	unsigned &						: polygon starting vertex in the array,		input/output
//		- size,		unsigned &						: number of vertices in the polygon,		input/output
//		- buffer,	float (*)[DIMENSION_T]			: storage buffer for the new vertices,		input/output
//		- nbuff,	unsigned &						: number of vertices in the storage buffer,	input/output
// returns
//		nil
// note
//		- plane vector defined as (n.x, n.y, n.z, -d), where |n| = 1;
//		- the polygon's original orientation (CW/CCW) is preserved in the result;
//		- degenerate results like a point or a line segment are possible!

template < unsigned DIMENSION_T >
void clip_by_plane(
	const float (&plane)[4],
	const float (**poly)[DIMENSION_T],
	unsigned& start,
	unsigned& size,
	float (*buffer)[DIMENSION_T],
	unsigned& nbuff)
{
	assert(3 <= DIMENSION_T);

	unsigned curr_v = start;
	unsigned prev_v = start + size - 1;
	unsigned counter = size;

	start += size;
	size = 0;

	while (counter--)
	{
		// calculate vertices' distances to the clipping plane
		const float curr_d = vect3::castU(plane).dot(vect3::castU(*poly[curr_v])) + plane[3];
		const float prev_d = vect3::castU(plane).dot(vect3::castU(*poly[prev_v])) + plane[3];

		if (curr_d < 0.f) // is current vertex outside?
		{
			if (prev_d > 0.f) // is previous vertex strictly inside?
			{
				vect< DIMENSION_T >::cast(buffer[nbuff]).interpolate(
					vect< DIMENSION_T >::cast(*poly[curr_v]),
					vect< DIMENSION_T >::cast(*poly[prev_v]),
					curr_d, prev_d);

				poly[start + size++] = &buffer[nbuff++];
			}
		}
		else // current vertex is inside
		{
			if (curr_d > 0.f &&	// is current vertex strictly inside and
				prev_d < 0.f)	// is previous vertex outside?
			{
				vect< DIMENSION_T >::cast(buffer[nbuff]).interpolate(
					vect< DIMENSION_T >::cast(*poly[curr_v]),
					vect< DIMENSION_T >::cast(*poly[prev_v]),
					curr_d, prev_d);

				poly[start + size++] = &buffer[nbuff++];
			}

			poly[start + size++] = poly[curr_v];
		}

		prev_v = curr_v++;
	}
}

} // namespace rend

#endif // rend_vect_H__
