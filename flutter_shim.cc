// Copyright...

#include "flutter/fml/logging.h"
#include "flutter/fml/trace_event.h"

namespace fml {

namespace {

const char* const kLogSeverityNames[LOG_NUM_SEVERITIES] = {"INFO", "WARNING",
                                                           "ERROR", "FATAL"};

const char* GetNameForLogSeverity(LogSeverity severity) {
  if (severity >= LOG_INFO && severity < LOG_NUM_SEVERITIES)
    return kLogSeverityNames[severity];
  return "UNKNOWN";
}

const char* StripDots(const char* path) {
  while (strncmp(path, "../", 3) == 0)
    path += 3;
  return path;
}

const char* StripPath(const char* path) {
  auto* p = strrchr(path, '/');
  if (p)
    return p + 1;
  else
    return path;
}

}  // namespace {}

LogMessage::LogMessage(LogSeverity severity,
                       const char* file,
                       int line,
                       const char* condition)
    : severity_(severity), file_(file), line_(line) {
  stream_ << "[";
  if (severity >= LOG_INFO)
    stream_ << GetNameForLogSeverity(severity);
  else
    stream_ << "VERBOSE" << -severity;
  stream_ << ":" << (severity > LOG_INFO ? StripDots(file_) : StripPath(file_))
          << "(" << line_ << ")] ";

  if (condition)
    stream_ << "Check failed: " << condition << ". ";

}

LogMessage::~LogMessage() {
  stream_ << std::endl;
  fprintf(stderr, "%s", stream_.str().c_str());
}

bool ShouldCreateLogMessage(LogSeverity severity) {
  return true;
}

namespace tracing {

void TraceEvent0(TraceArg category_group, TraceArg name) {
}

void TraceEvent1(TraceArg category_group,
                 TraceArg name,
                 TraceArg arg1_name,
                 TraceArg arg1_val) {
}

void TraceEventEnd(TraceArg name) {
}

}  // namespace tracing

}  // namespace fml
