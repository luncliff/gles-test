// we have to pre-include all headers rendVect.hpp includes, lest each of those headers
// end up in only one of the namespaces we include rendVect.hpp in, and that won't work
#include <cmath>
#include <cstring>
#include <cassert>

// include the rest of the headers we use in this TU
#include <iostream>
#include <iomanip>
#include <time.h>
#include <stdint.h>

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

#undef REND_MADD__
#undef REND_MATX_MUL_V2__

namespace a
{
#include "rendVect.hpp"
}

#undef rend_vect_H__
#define REND_MADD__
#define REND_MATX_MUL_V2__

namespace b
{
#include "rendVect.hpp"
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


a::rend::matx4 ma[2] __attribute__ ((aligned (16)));
a::rend::matx4 ra[3] __attribute__ ((aligned (16)));

b::rend::matx4 mb[2] __attribute__ ((aligned (16)));
b::rend::matx4 rb[3] __attribute__ ((aligned (16)));


int main(
	int argc,
	char** argv)
{
	bool verbose = false;
	bool timing = false;
	unsigned reps = 1;

	for (int i = 1; i < argc; ++i)
	{
		if (0 == strcmp(argv[i], "verbose"))
		{
			verbose = true;
			continue;
		}

		if (0 == strcmp(argv[i], "timing"))
		{
			timing = true;
			reps = unsigned(1e+7);
			continue;
		}

		std::cerr << "usage: " << argv[0] << " [verbose] [timing]" << std::endl;
		return -1;
	}

	float factor;

	std::cout << "enter 1.0: ";
	std::cin >> factor; // pseudo-entropy factor, must be 1

	const a::rend::vect3& va = a::rend::vect3(1.f, 1.f, 1.f).normalise().mul(factor);
	const b::rend::vect3& vb = b::rend::vect3(1.f, 1.f, 1.f).normalise().mul(factor * factor);

	ma[0] = a::rend::matx4().rotate(M_PI_2, va[0], va[1], va[2]);
	ma[1] = a::rend::matx4().rotate(-M_PI_2, va[0], va[1], va[2]);

	mb[0] = b::rend::matx4().rotate(M_PI_2, vb[0], vb[1], vb[2]);
	mb[1] = b::rend::matx4().rotate(-M_PI_2, vb[0], vb[1], vb[2]);

	unsigned offset = unsigned(factor) - 1; // pseudo-offset, should be 0
	const uint64_t t0 = timer_nsec();

	do
	{
		const unsigned offs0 = offset + 0;
		const unsigned offs1 = offset + 1;
		const unsigned offs2 = offset + 2;

		ra[offs0] = a::rend::matx4().mul(ma[offs0], ma[offs1]);
		ra[offs1] = a::rend::matx4(ma[offs0]).mur(ma[offs1]);
		ra[offs2] = a::rend::matx4(ma[offs1]).mul(ma[offs0]);

		rb[offs0] = b::rend::matx4().mul(mb[offs0], mb[offs1]);
		rb[offs1] = b::rend::matx4(mb[offs0]).mur(mb[offs1]);
		rb[offs2] = b::rend::matx4(mb[offs1]).mul(mb[offs0]);

		offset += offset;
	}
	while (--reps);

	const uint64_t dt = timer_nsec() - t0;

	if (timing)
	{
		const double sec = double(dt) * 1e-9;
		std::cout << "elapsed time: " << sec << " s" << std::endl;
	}

	assert(
		sizeof(ra) / sizeof(ra[0]) ==
		sizeof(rb) / sizeof(rb[0]));
	assert(
		sizeof(ra[0].decastF()) / sizeof(ra[0].decastF()[0]) ==
		sizeof(rb[0].decastF()) / sizeof(rb[0].decastF()[0]));

	bool err = false;

	for (unsigned i = 0; i < sizeof(ra) / sizeof(ra[0]); ++i)
	{
		if (verbose)
		{
			std::cout <<
				formatter(ra[i][0][0]) << "\t" <<
				formatter(ra[i][0][1]) << "\t" <<
				formatter(ra[i][0][2]) << "\t" <<
				formatter(ra[i][0][3]) << "\n" <<
				formatter(ra[i][1][0]) << "\t" <<
				formatter(ra[i][1][1]) << "\t" <<
				formatter(ra[i][1][2]) << "\t" <<
				formatter(ra[i][1][3]) << "\n" <<
				formatter(ra[i][2][0]) << "\t" <<
				formatter(ra[i][2][1]) << "\t" <<
				formatter(ra[i][2][2]) << "\t" <<
				formatter(ra[i][2][3]) << "\n" <<
				formatter(ra[i][3][0]) << "\t" <<
				formatter(ra[i][3][1]) << "\t" <<
				formatter(ra[i][3][2]) << "\t" <<
				formatter(ra[i][3][3]) << "\n" <<
				std::endl;
			std::cout <<
				formatter(rb[i][0][0]) << "\t" <<
				formatter(rb[i][0][1]) << "\t" <<
				formatter(rb[i][0][2]) << "\t" <<
				formatter(rb[i][0][3]) << "\n" <<
				formatter(rb[i][1][0]) << "\t" <<
				formatter(rb[i][1][1]) << "\t" <<
				formatter(rb[i][1][2]) << "\t" <<
				formatter(rb[i][1][3]) << "\n" <<
				formatter(rb[i][2][0]) << "\t" <<
				formatter(rb[i][2][1]) << "\t" <<
				formatter(rb[i][2][2]) << "\t" <<
				formatter(rb[i][2][3]) << "\n" <<
				formatter(rb[i][3][0]) << "\t" <<
				formatter(rb[i][3][1]) << "\t" <<
				formatter(rb[i][3][2]) << "\t" <<
				formatter(rb[i][3][3]) << "\n" <<
				std::endl;
		}

		for (unsigned j = 0; j < sizeof(ra[i]) / sizeof(ra[i][0][0]); ++j)
		{
			const float abs_diff = fabs(ra[i][j / 4][j % 4] - rb[i][j / 4][j % 4]);
			const float eps = 1e-7;

			if (abs_diff > eps)
			{
				std::cout << "element " << j << " has abs diff " << abs_diff << std::endl;
				err = true;
			}
		}
	}

	return err ? 1 : 0;
}
