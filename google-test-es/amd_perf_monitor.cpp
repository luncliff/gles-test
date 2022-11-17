#if defined(PLATFORM_GLX)

#include <GL/gl.h>
#include <GL/glext.h>

#else

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#endif

#include <math.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "get_proc_address.hpp"
#include "amd_perf_monitor.hpp"

namespace amd_perf_monitor
{

static bool g_extension_checked;
static bool g_extension_available;

#if GL_AMD_performance_monitor != 0

static GLuint g_monitor;
static GLuint g_group = -1U;

// trace certain counters from the RB_SAMPLES group of AMD Z430
enum
{
	Z430_RB_TOTAL_SAMPLES = 0,
	Z430_RB_ZPASS_SAMPLES,
	Z430_RB_ZFAIL_SAMPLES,
	Z430_RB_SFAIL_SAMPLES,

	Z430_RB_count,
	Z430_RB_force_type = -1U
};

static GLuint g_counter[Z430_RB_count];

static PFNGLGETPERFMONITORGROUPSAMDPROC
	gglGetPerfMonitorGroupsAMD;

static PFNGLGETPERFMONITORCOUNTERSAMDPROC
	gglGetPerfMonitorCountersAMD;

static PFNGLGETPERFMONITORGROUPSTRINGAMDPROC
	gglGetPerfMonitorGroupStringAMD;

static PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC
	gglGetPerfMonitorCounterStringAMD;

static PFNGLGETPERFMONITORCOUNTERINFOAMDPROC
	gglGetPerfMonitorCounterInfoAMD;

static PFNGLGENPERFMONITORSAMDPROC
	gglGenPerfMonitorsAMD;

static PFNGLDELETEPERFMONITORSAMDPROC
	gglDeletePerfMonitorsAMD;

static PFNGLSELECTPERFMONITORCOUNTERSAMDPROC
	gglSelectPerfMonitorCountersAMD;

static PFNGLBEGINPERFMONITORAMDPROC
	gglBeginPerfMonitorAMD;

static PFNGLENDPERFMONITORAMDPROC
	gglEndPerfMonitorAMD;

static PFNGLGETPERFMONITORCOUNTERDATAAMDPROC
	gglGetPerfMonitorCounterDataAMD;


static const char*
string_from_AMD_counter_type(
	const GLenum type)
{
	switch (type) {

	case GL_UNSIGNED_INT:
		return "GL_UNSIGNED_INT";

	case GL_UNSIGNED_INT64_AMD:
		return "GL_UNSIGNED_INT64_AMD";

	case GL_PERCENTAGE_AMD:
		return "GL_PERCENTAGE_AMD";

	case GL_FLOAT:
		return "GL_FLOAT";
	}

	return "unknown AMD counter type";
}


static std::string
string_from_AMD_counter_range(
	const GLenum type,
	void* range)
{
	std::ostringstream s;

	const uint64_t& r0 = reinterpret_cast< uint64_t* >(range)[0];
	const uint64_t& r1 = reinterpret_cast< uint64_t* >(range)[1];

	switch (type) {

	case GL_UNSIGNED_INT:
		s << "[" <<
			reinterpret_cast< GLuint* >(range)[0] << ", " <<
			reinterpret_cast< GLuint* >(range)[1] << "]";

		return s.str();

	case GL_UNSIGNED_INT64_AMD:
		if (r0)
		{
			const unsigned log2 = unsigned(ceil(log(double(r0)) / log(2.0)));
			const unsigned rem = unsigned((1ULL << log2) - r0);

			if (rem)
				s << "[2^" << log2 << " - " << rem << ", ";
			else
				s << "[2^" << log2 << ", ";
		}
		else
			s << "[0, ";

		if (r1)
		{
			const unsigned log2 = unsigned(ceil(log(double(r1)) / log(2.0)));
			const unsigned rem = unsigned((1ULL << log2) - r1);

			if (rem)
				s << "2^" << log2 << " - " << rem << "]";
			else
				s << "2^" << log2 << "]";
		}
		else
			s << "0]";

		return s.str();

	case GL_PERCENTAGE_AMD:
		return "[0.f, 100.f]";

	case GL_FLOAT:
		s << "[" <<
			reinterpret_cast< GLfloat* >(range)[0] << ", " <<
			reinterpret_cast< GLfloat* >(range)[1] << "]";

		return s.str();
	}

	return "unknown AMD counter type";
}

#endif // GL_AMD_performance_monitor

void
peek_performance_monitor(
	const bool print_perf_counters)
{
#if GL_AMD_performance_monitor != 0

	if (!g_extension_available)
		return;

	GLint ngroups;
	static GLuint group[128];

	gglGetPerfMonitorGroupsAMD(&ngroups, GLsizei(sizeof(group) / sizeof(group[0])), group);

	if (print_perf_counters)
		std::cout << "\nAMD_performance_monitor groups count: " << ngroups << std::endl;

	if (size_t(ngroups) > sizeof(group) / sizeof(group[0]))
	{
		std::cout << "warning: number of groups exceeds expectations; discarding excess" << std::endl;
		ngroups = sizeof(group) / sizeof(group[0]);
	}

	for (GLint i = 0; i < ngroups; ++i)
	{
		static char group_name[256];
		GLsizei length;

		gglGetPerfMonitorGroupStringAMD(group[i], GLsizei(sizeof(group_name) / sizeof(group_name[0])), &length, group_name);

		if (print_perf_counters)
			std::cout << "group " << i << ": " << group_name << ", id: " << group[i] << std::endl;

		GLint ncounters;
		GLint max_active_counters;

		static GLuint counter[512];

		gglGetPerfMonitorCountersAMD(group[i], &ncounters, &max_active_counters,
			GLsizei(sizeof(counter) / sizeof(counter[0])), counter);

		if (print_perf_counters)
		{
			std::cout << "number of counters: " << ncounters <<
				"\nmax number of active counters: " << max_active_counters << std::endl;
		}

		if (size_t(ncounters) > sizeof(counter) / sizeof(counter[0]))
		{
			std::cout << "warning: number of counters exceeds expectations; discarding excess" << std::endl;
			ncounters = sizeof(counter) / sizeof(counter[0]);
		}

		bool group_sought = false;

		if (!strcmp(group_name, "RB_SAMPLES"))
		{
			g_group = group[i];
			group_sought = true;
		}

		for (GLint j = 0; j < ncounters; ++j)
		{
			static char counter_name[256];
			GLsizei length;

			gglGetPerfMonitorCounterStringAMD(group[i], counter[j],
				GLsizei(sizeof(counter_name) / sizeof(counter_name[0])), &length, counter_name);

			GLenum counter_type;
			uint64_t counter_range[2];

			gglGetPerfMonitorCounterInfoAMD(group[i], counter[j], GL_COUNTER_TYPE_AMD, &counter_type);

			if (GL_PERCENTAGE_AMD != counter_type)
				gglGetPerfMonitorCounterInfoAMD(group[i], counter[j], GL_COUNTER_RANGE_AMD, counter_range);

			if (print_perf_counters)
			{
				std::cout << "\tcounter " << j << ": " << counter_name <<
					", id: " << counter[j] <<
					", type: " << string_from_AMD_counter_type(counter_type) <<
					", range: " << string_from_AMD_counter_range(counter_type, counter_range) << std::endl;
			}

			if (group_sought)
			{
				if (!strcmp(counter_name, "RB_TOTAL_SAMPLES"))
					g_counter[Z430_RB_TOTAL_SAMPLES] = counter[j];
				else
				if (!strcmp(counter_name, "RB_ZPASS_SAMPLES"))
					g_counter[Z430_RB_ZPASS_SAMPLES] = counter[j];
				else
				if (!strcmp(counter_name, "RB_ZFAIL_SAMPLES"))
					g_counter[Z430_RB_ZFAIL_SAMPLES] = counter[j];
				else
				if (!strcmp(counter_name, "RB_SFAIL_SAMPLES"))
					g_counter[Z430_RB_SFAIL_SAMPLES] = counter[j];
			}
		}
	}

	if (g_group != -1U)
	{
		std::cout << "tracing AMD_performance_monitor counters:\n"
			"\tgroup id: " << g_group << ", counter id: " << g_counter[Z430_RB_TOTAL_SAMPLES] << "\n"
			"\tgroup id: " << g_group << ", counter id: " << g_counter[Z430_RB_ZPASS_SAMPLES] << "\n"
			"\tgroup id: " << g_group << ", counter id: " << g_counter[Z430_RB_ZFAIL_SAMPLES] << "\n"
			"\tgroup id: " << g_group << ", counter id: " << g_counter[Z430_RB_SFAIL_SAMPLES] << std::endl;

		gglGenPerfMonitorsAMD(GLsizei(1), &g_monitor);

		gglSelectPerfMonitorCountersAMD(
			g_monitor, GL_TRUE, g_group, GLint(sizeof(g_counter) / sizeof(g_counter[0])), g_counter);

		if (glGetError() != GL_NO_ERROR)
		{
			std::cerr << "failed to activate one or more AMD_performance_monitor counters" << std::endl;

			gglDeletePerfMonitorsAMD(GLsizei(1), &g_monitor);
			g_monitor = 0;
		}
		else
			gglBeginPerfMonitorAMD(g_monitor);
	}

#endif // GL_AMD_performance_monitor
}


void
depeek_performance_monitor()
{
#if GL_AMD_performance_monitor != 0

	if (g_monitor == 0)
		return;

	gglEndPerfMonitorAMD(g_monitor);

	static GLuint data[16];
	GLsizei bytes_of_data;

	gglGetPerfMonitorCounterDataAMD(
		g_monitor, GL_PERFMON_RESULT_AMD, GLsizei(sizeof(data)), data, &bytes_of_data);

	if (bytes_of_data == sizeof(data))
	{
		std::cout << "traced AMD_performance_monitor counters:\n"
			"\tgroup id: " << data[ 0] << ", counter id: " << data[ 1] << ", value: " <<
			*reinterpret_cast< uint64_t* >(data +  2) << "\n"
			"\tgroup id: " << data[ 4] << ", counter id: " << data[ 5] << ", value: " <<
			*reinterpret_cast< uint64_t* >(data +  6) << "\n"
			"\tgroup id: " << data[ 8] << ", counter id: " << data[ 9] << ", value: " <<
			*reinterpret_cast< uint64_t* >(data + 10) << "\n"
			"\tgroup id: " << data[12] << ", counter id: " << data[13] << ", value: " <<
			*reinterpret_cast< uint64_t* >(data + 14) << std::endl;
	}
	else
		std::cerr << "failure reading AMD_performance_monitor counters - invalid data size" << std::endl;

	gglSelectPerfMonitorCountersAMD(
		g_monitor, GL_FALSE, g_group, GLint(sizeof(g_counter) / sizeof(g_counter[0])), g_counter);

	gglDeletePerfMonitorsAMD(GLsizei(1), &g_monitor);
	g_monitor = 0;

#endif // GL_AMD_performance_monitor
}

bool
load_extension()
{
	if (g_extension_checked)
		return g_extension_available;

	g_extension_checked = true;
	const char ext_name_string[] = "GL_AMD_performance_monitor";

#if defined(PLATFORM_GLX)

	GLint num_extensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);

	GLuint i = 0;
	for (; i != (GLuint) num_extensions; ++i)
		if (0 == strcmp((const char*) glGetStringi(GL_EXTENSIONS, i), ext_name_string))
			break;

	if (i == (GLuint) num_extensions)
		return false;

#else

	const GLubyte* extensions = glGetString(GL_EXTENSIONS);

	if (!strstr((const char*) extensions, ext_name_string))
		return false;

#endif

#if GL_AMD_performance_monitor != 0

	g_extension_available = true;

	g_extension_available = g_extension_available && 0 != (gglGetPerfMonitorGroupsAMD =
		(PFNGLGETPERFMONITORGROUPSAMDPROC) getProcAddress("glGetPerfMonitorGroupsAMD"));

	g_extension_available = g_extension_available && 0 != (gglGetPerfMonitorCountersAMD =
		(PFNGLGETPERFMONITORCOUNTERSAMDPROC) getProcAddress("glGetPerfMonitorCountersAMD"));

	g_extension_available = g_extension_available && 0 != (gglGetPerfMonitorGroupStringAMD =
		(PFNGLGETPERFMONITORGROUPSTRINGAMDPROC) getProcAddress("glGetPerfMonitorGroupStringAMD"));

	g_extension_available = g_extension_available && 0 != (gglGetPerfMonitorCounterStringAMD =
		(PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC) getProcAddress("glGetPerfMonitorCounterStringAMD"));

	g_extension_available = g_extension_available && 0 != (gglGetPerfMonitorCounterInfoAMD =
		(PFNGLGETPERFMONITORCOUNTERINFOAMDPROC) getProcAddress("glGetPerfMonitorCounterInfoAMD"));

	g_extension_available = g_extension_available && 0 != (gglGenPerfMonitorsAMD =
		(PFNGLGENPERFMONITORSAMDPROC) getProcAddress("glGenPerfMonitorsAMD"));

	g_extension_available = g_extension_available && 0 != (gglDeletePerfMonitorsAMD =
		(PFNGLDELETEPERFMONITORSAMDPROC) getProcAddress("glDeletePerfMonitorsAMD"));

	g_extension_available = g_extension_available && 0 != (gglSelectPerfMonitorCountersAMD =
		(PFNGLSELECTPERFMONITORCOUNTERSAMDPROC) getProcAddress("glSelectPerfMonitorCountersAMD"));

	g_extension_available = g_extension_available && 0 != (gglBeginPerfMonitorAMD =
		(PFNGLBEGINPERFMONITORAMDPROC) getProcAddress("glBeginPerfMonitorAMD"));

	g_extension_available = g_extension_available && 0 != (gglEndPerfMonitorAMD =
		(PFNGLENDPERFMONITORAMDPROC) getProcAddress("glEndPerfMonitorAMD"));

	g_extension_available = g_extension_available && 0 != (gglGetPerfMonitorCounterDataAMD =
		(PFNGLGETPERFMONITORCOUNTERDATAAMDPROC) getProcAddress("glGetPerfMonitorCounterDataAMD"));

#endif // GL_AMD_performance_monitor

	return g_extension_available;
}

} // namespace amd_perf_monitor
