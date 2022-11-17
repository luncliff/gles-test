#include <iostream>
#include <fstream>
#include <iomanip>
#include <cassert>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>

// SIMD_AUTOVECT enum

#define SIMD_1WAY		1 // no SIMD; use scalars instead
#define SIMD_2WAY		2
#define SIMD_4WAY		3

// SIMD_INTRINSICS enum

#define SIMD_ALTIVEC	1
#define SIMD_SSE		2
#define SIMD_NEON		3

#undef SIMD_INTRINSICS
#if SIMD_AUTOVECT == 0
	#if __ALTIVEC__ == 1
		#define SIMD_INTRINSICS SIMD_ALTIVEC
	#elif __SSE__ == 1
		#define SIMD_INTRINSICS SIMD_SSE
	#elif __ARM_NEON__ == 1
		#define SIMD_INTRINSICS SIMD_NEON
	#endif
#endif

#if SIMD_INTRINSICS == SIMD_ALTIVEC
	#include <altivec.h>
#elif SIMD_INTRINSICS == SIMD_SSE
	#include <xmmintrin.h>
#elif SIMD_INTRINSICS == SIMD_NEON
	#include <arm_neon.h>
#endif

static uint64_t
timer_nsec()
{
#if defined(CLOCK_MONOTONIC_RAW)
	const clockid_t clockid = CLOCK_MONOTONIC_RAW;
#else
	const clockid_t clockid = CLOCK_MONOTONIC;
#endif

	timespec t;
	clock_gettime(clockid, &t);

	return t.tv_sec * 1000000000ULL + t.tv_nsec;
}


class matx4
{
#if SIMD_types_for_the_autovectorizer
#error

#elif SIMD_AUTOVECT == SIMD_2WAY
	typedef float vect2_float __attribute__ ((vector_size (8)));

#elif SIMD_AUTOVECT == SIMD_4WAY
	typedef float vect4_float __attribute__ ((vector_size (16)));

#endif // SIMD_types_for_the_autovectorizer

	union
	{
		float m[4][4];

#if SIMD_aliases_to_the_above_C_array
#error

#elif SIMD_AUTOVECT == SIMD_2WAY
		vect2_float n[4][2];

#elif SIMD_AUTOVECT == SIMD_4WAY
		vect4_float n[4];

#elif SIMD_INTRINSICS == SIMD_ALTIVEC
		vector float n[4];

#elif SIMD_INTRINSICS == SIMD_SSE
		__m128 n[4];

#elif SIMD_INTRINSICS == SIMD_NEON
		float32x4_t n[4];

#endif // SIMD_aliases_to_the_above_C_array
	};

public:
	matx4() {}
	matx4(
		const float m00, const float m01, const float m02, const float m03,
		const float m10, const float m11, const float m12, const float m13,
		const float m20, const float m21, const float m22, const float m23,
		const float m30, const float m31, const float m32, const float m33)
	{
		m[0][0] = m00;
		m[0][1] = m01;
		m[0][2] = m02;
		m[0][3] = m03;

		m[1][0] = m10;
		m[1][1] = m11;
		m[1][2] = m12;
		m[1][3] = m13;

		m[2][0] = m20;
		m[2][1] = m21;
		m[2][2] = m22;
		m[2][3] = m23;

		m[3][0] = m30;
		m[3][1] = m31;
		m[3][2] = m32;
		m[3][3] = m33;
	}

	typedef const float (&decast)[4][4];
	operator decast () const
	{
		return m;
	}

	float (& dekast())[4][4]
	{
		return m;
	}

	static matx4&
	cast(
		float (&mat)[4][4])
	{
		return *reinterpret_cast< matx4* >(&mat);
	}

	static const matx4&
	cast(
		const float (&mat)[4][4])
	{
		return *reinterpret_cast< const matx4* >(&mat);
	}

	matx4&
	mul(
		const matx4& mat0,
		const matx4& mat1)
	{
		for (unsigned i = 0; i < 4; ++i)
		{
#if SIMD_preamble_per_output_row
#error

#elif SIMD_INTRINSICS == SIMD_ALTIVEC
			vector float const e0 = vec_splat(mat0.n[i], 0);
			n[i] = vec_madd(e0, mat1.n[0], ((vector float) { -0.f, -0.f, -0.f -0.f }));

			vector float const e1 = vec_splat(mat0.n[i], 1);
			n[i] = vec_madd(e1, mat1.n[1], n[i]);

			vector float const e2 = vec_splat(mat0.n[i], 2);
			n[i] = vec_madd(e2, mat1.n[2], n[i]);

			vector float const e3 = vec_splat(mat0.n[i], 3);
			n[i] = vec_madd(e3, mat1.n[3], n[i]);

#elif SIMD_INTRINSICS == SIMD_SSE
			__m128 const e0 = _mm_shuffle_ps(mat0.n[i], mat0.n[i], _MM_SHUFFLE(0, 0, 0, 0));
			n[i] = _mm_mul_ps(e0, mat1.n[0]);

			__m128 const e1 = _mm_shuffle_ps(mat0.n[i], mat0.n[i], _MM_SHUFFLE(1, 1, 1, 1));
			n[i] = _mm_add_ps(_mm_mul_ps(e1, mat1.n[1]), n[i]);

			__m128 const e2 = _mm_shuffle_ps(mat0.n[i], mat0.n[i], _MM_SHUFFLE(2, 2, 2, 2));
			n[i] = _mm_add_ps(_mm_mul_ps(e2, mat1.n[2]), n[i]);

			__m128 const e3 = _mm_shuffle_ps(mat0.n[i], mat0.n[i], _MM_SHUFFLE(3, 3, 3, 3));
			n[i] = _mm_add_ps(_mm_mul_ps(e3, mat1.n[3]), n[i]);

#elif SIMD_INTRINSICS == SIMD_NEON
			n[i] = vmulq_n_f32(mat1.n[0], mat0.m[i][0]);
			n[i] = vmlaq_n_f32(n[i], mat1.n[1], mat0.m[i][1]);
			n[i] = vmlaq_n_f32(n[i], mat1.n[2], mat0.m[i][2]);
			n[i] = vmlaq_n_f32(n[i], mat1.n[3], mat0.m[i][3]);

#elif SIMD_AUTOVECT == SIMD_2WAY
			const vect2_float e0 =
			{
				mat0.m[i][0],
				mat0.m[i][0]
			};
			n[i][0] = e0 * mat1.n[0][0];
			n[i][1] = e0 * mat1.n[0][1];

#elif SIMD_AUTOVECT == SIMD_4WAY
			const vect4_float e0 =
			{
				mat0.m[i][0],
				mat0.m[i][0],
				mat0.m[i][0],
				mat0.m[i][0]
			};
			n[i] = e0 * mat1.n[0];

#else // scalar
			for (unsigned j = 0; j < 4; ++j)
				m[i][j] = mat0[i][0] * mat1[0][j];

#endif // SIMD_preamble_per_output_row

// Manually-emitted intrinsics are done with their work per
// this output row; following is for the autovectorizer alone.

#if SIMD_INTRINSICS == 0
			for (unsigned j = 1; j < 4; ++j) 
			{

#if SIMD_process_rows_1_through_4
#error

#elif SIMD_AUTOVECT == SIMD_2WAY
				const vect2_float ej =
				{
					mat0.m[i][j],
					mat0.m[i][j]
				};
				n[i][0] += ej * mat1.n[j][0];
				n[i][1] += ej * mat1.n[j][1];

#elif SIMD_AUTOVECT == SIMD_4WAY
				const vect4_float ej =
				{
					mat0.m[i][j],
					mat0.m[i][j],
					mat0.m[i][j],
					mat0.m[i][j]
				};
				n[i] += ej * mat1.n[j];

#else // scalar
				for (unsigned k = 0; k < 4; ++k)
					m[i][k] += mat0[i][j] * mat1[j][k];

#endif // SIMD_process_rows_1_through_4
			}

#endif // SIMD_INTRINSICS == 0
		}

		return *this;
	}
};

std::istream& operator >> (
	std::istream& str,
	matx4& a)
{
	float t[4][4];

	str >> t[0][0];
	str >> t[0][1];
	str >> t[0][2];
	str >> t[0][3];

	str >> t[1][0];
	str >> t[1][1];
	str >> t[1][2];
	str >> t[1][3];

	str >> t[2][0];
	str >> t[2][1];
	str >> t[2][2];
	str >> t[2][3];

	str >> t[3][0];
	str >> t[3][1];
	str >> t[3][2];
	str >> t[3][3];

	a = matx4::cast(t);

	return str;
}


class formatter
{
	const float m;

public:
	formatter(const float& a)
	: m(a)
	{}

	float get() const
	{
		return m;
	}
};

std::ostream& operator << (
	std::ostream& str,
	const formatter& a)
{
	return str << std::setw(12) << std::setfill('_') << a.get();
}


std::ostream& operator << (
	std::ostream& str,
	const matx4& a)
{
	return str <<
		formatter(a[0][0]) << " " << formatter(a[0][1]) << " " << formatter(a[0][2]) << " " << formatter(a[0][3]) << '\n' <<
		formatter(a[1][0]) << " " << formatter(a[1][1]) << " " << formatter(a[1][2]) << " " << formatter(a[1][3]) << '\n' <<
		formatter(a[2][0]) << " " << formatter(a[2][1]) << " " << formatter(a[2][2]) << " " << formatter(a[2][3]) << '\n' <<
		formatter(a[3][0]) << " " << formatter(a[3][1]) << " " << formatter(a[3][2]) << " " << formatter(a[3][3]) << '\n' <<
		std::endl;
}

static const size_t reps = unsigned(1e+7) * 6;
static const size_t nthreads = SIMD_NUM_THREADS;
static const size_t one_less = nthreads - 1;

matx4 ma[2] __attribute__ ((aligned (16)));
matx4 ra[nthreads] __attribute__ ((aligned (16)));

enum {
	BARRIER_START,
	BARRIER_FINISH,
	BARRIER_COUNT
};

static pthread_barrier_t barrier[BARRIER_COUNT];

struct compute_arg
{
	pthread_t thread;
	size_t id;
	size_t offset;

	compute_arg() :
		thread(0)
	{}

	compute_arg(
		const size_t arg_id,
		const size_t arg_offs) :
		thread(0),
		id(arg_id),
		offset(arg_offs)
	{}
};

static void*
compute(
	void* arg)
{
	const compute_arg* const carg = reinterpret_cast< compute_arg* >(arg);
	const size_t id = carg->id;
	const size_t offset = carg->offset;

	pthread_barrier_wait(barrier + BARRIER_START);

	for (size_t i = 0; i < reps; ++i)
	{
		const size_t offs0 = i * offset + 0;
		const size_t offs1 = i * offset + 1;

		ra[id + offs0] = matx4().mul(ma[offs0], ma[offs1]);
	}

	pthread_barrier_wait(barrier + BARRIER_FINISH);

	return 0;
}


class workforce_t
{
	compute_arg record[one_less];
	bool successfully_init;

public:
	workforce_t(const size_t offset);
	~workforce_t();

	bool is_successfully_init() const
	{
		return successfully_init;
	}
};


static void
report_err(
	const char* const func,
	const int line,
	const size_t counter,
	const int err)
{
	std::cerr << func << ':' << line << ", i: "
		<< counter << ", err: " << err << std::endl;
}


workforce_t::workforce_t(
	const size_t offset) :
	successfully_init(false)
{
	for (size_t i = 0; i < sizeof(barrier) / sizeof(barrier[0]); ++i)
	{
		const int r = pthread_barrier_init(barrier + i, NULL, nthreads);

		if (0 != r)
		{
			report_err(__FUNCTION__, __LINE__, i, r);
			return;
		}
	}

	for (size_t i = 0; i < one_less; ++i)
	{
		record[i] = compute_arg(i + 1, offset);

		const int r = pthread_create(&record[i].thread, NULL, compute, record + i);

		if (0 != r)
		{
			report_err(__FUNCTION__, __LINE__, i, r);
			return;
		}
	}

	successfully_init = true;
}


workforce_t::~workforce_t()
{
	for (size_t i = 0; i < sizeof(barrier) / sizeof(barrier[0]); ++i)
	{
		const int r = pthread_barrier_destroy(barrier + i);

		if (0 != r)
			report_err(__FUNCTION__, __LINE__, i, r);
	}

	for (size_t i = 0; i < one_less && 0 != record[i].thread; ++i)
	{
		const int r = pthread_join(record[i].thread, NULL);

		if (0 != r)
			report_err(__FUNCTION__, __LINE__, i, r);
	}
}


int
main(
	int argc,
	char** argv)
{
	size_t obfuscator; // invariance obfuscator
	std::ifstream in("vect.input");

	if (in.is_open())
	{
		in >> ma[0];
		in >> ma[1];
		in >> obfuscator;
		in.close();
	}
	else
	{
		std::cout << "enter ma0: ";
		std::cin >> ma[0];
		std::cout << "enter ma1: ";
		std::cin >> ma[1];
		std::cout << "enter 0: ";
		std::cin >> obfuscator;
	}

	const workforce_t workforce(obfuscator);

	if (!workforce.is_successfully_init())
	{
		std::cerr << "failed to raise workforce; bailing out" << std::endl;
		return -1;
	}

	// let the workforce start their engines
	const timespec ts = { 0, one_less * 100000000 };
	nanosleep(&ts, NULL);

	compute_arg carg(0, obfuscator);

	const uint64_t t0 = timer_nsec();

	compute(&carg);

	const uint64_t dt = timer_nsec() - t0;
	const double sec = double(dt) * 1e-9;

	std::cout << "elapsed time: " << sec << " s" << std::endl;

	for (size_t i = 0; i < sizeof(ra) / sizeof(ra[0]); ++i)
		std::cout << ra[i];

	return 0;
}
