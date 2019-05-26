#include "logger.hpp"

#include "common.hpp"

#ifdef WINDOWS_PLATFORM
#include <Windows.h>
#endif

namespace rf
{
namespace
{
char const * const kLogFileName = "log.txt";
std::ofstream g_logFile;
std::mutex g_logMutex;

std::string SeverityToString(Logger::Severity s)
{
  switch (s)
  {
    case Logger::Severity::Error:
      return "Error: ";
    case Logger::Severity::Warning:
      return "Warning: ";
    case Logger::Severity::Info:
      return "Info: ";
  }
  CHECK(false, "Unsupported severity type");
  return {};
}
}  // namespace

unsigned char Logger::outputFlags = static_cast<unsigned char>(Logger::OutputFlags::Console);

void Logger::ToLog(Severity severity, std::string const & message)
{
  std::lock_guard<std::mutex> lock(g_logMutex);
  ToLogImpl(severity, message);
}

void Logger::ToLogImpl(Severity severity, std::string const & message)
{
  if ((outputFlags & static_cast<unsigned char>(OutputFlags::Console)) != 0)
  {
    std::cout << SeverityToString(severity) << message << std::endl;
#ifdef WINDOWS_PLATFORM
    OutputDebugStringA((SeverityToString(severity) + message + "\n").c_str());
#endif
  }
  if ((outputFlags & static_cast<unsigned char>(OutputFlags::File)) != 0)
  {
    if (g_logFile.is_open())
      g_logFile << SeverityToString(severity) << message << std::endl;
  }
}

void Logger::ToLogWithFormat(Severity severity, char const * format, ...)
{
  std::lock_guard<std::mutex> lock(g_logMutex);
  std::string fmt(format);
  auto const n = std::count(fmt.begin(), fmt.end(), '%');
  if (n > 0)
  {
    uint32_t constexpr kSize = 2048;
    static char buf[kSize];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, kSize, format, args);
    va_end(args);

    ToLogImpl(severity, buf);
  }
  else
  {
    ToLogImpl(severity, format);
  }
}

void Logger::SetOutputFlags(unsigned char flags)
{
  outputFlags |= flags;
}

void Logger::Start(unsigned char flags)
{
  std::lock_guard<std::mutex> lock(g_logMutex);
  SetOutputFlags(flags);
  if ((outputFlags & (unsigned char)OutputFlags::File) != 0)
    g_logFile.open(kLogFileName);
}

void Logger::Finish()
{
  std::lock_guard<std::mutex> lock(g_logMutex);
  if (g_logFile.is_open())
  {
    g_logFile.flush();
    g_logFile.close();
  }
}

void Logger::Flush()
{
  std::lock_guard<std::mutex> lock(g_logMutex);
  if (g_logFile.is_open())
    g_logFile.flush();
}
}  // namespace rf