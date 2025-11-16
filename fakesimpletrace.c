#include "simpletrace.h"

void trace0(TraceEventID event)
{

}

void trace1(TraceEventID event, uint64_t x1)
{

}

void trace2(TraceEventID event, uint64_t x1, uint64_t x2)
{

}

void trace3(TraceEventID event, uint64_t x1, uint64_t x2, uint64_t x3)
{

}

void trace4(TraceEventID event, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4)
{

}

void trace5(TraceEventID event, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4, uint64_t x5)
{

}

void trace6(TraceEventID event, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4, uint64_t x5, uint64_t x6)
{

}
void st_print_trace(FILE* stream, int (*stream_printf)(FILE* stream, const char* fmt, ...))
{

}

void st_print_trace_events(FILE* stream, int (*stream_printf)(FILE* stream, const char* fmt, ...))
{

}

void st_change_trace_event_state(const char* tname, bool tstate)
{

}

void st_print_trace_file_status(FILE* stream, int (*stream_printf)(FILE* stream, const char* fmt, ...))
{

}

void st_set_trace_file_enabled(bool enable)
{

}

bool st_set_trace_file(const char* file)
{

}

void st_flush_trace_buffer(void)
{

}