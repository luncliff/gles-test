#ifndef AMD_performance_monitor_H__
#define AMD_performance_monitor_H__

////////////////////////////////////////////////////////////////////////////////////////////////////
// GL_AMD_performance_monitor
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace amd_perf_monitor
{

bool
load_extension();

void
peek_performance_monitor(
	const bool print_perf_counters);

void
depeek_performance_monitor();

} // namespace amd_perf_monitor

#endif // AMD_performance_monitor_H__
