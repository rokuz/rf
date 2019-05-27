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
}  // namespace

std::string SeverityToString(Logger::Severity s)
{
  switch (s)
  {
    case Logger::Severity::Error:
      return "Error";
    case Logger::Severity::Warning:
      return "Warning";
    case Logger::Severity::Info:
      return "Info";
  }
  CHECK(false, "Unsupported severity type");
  return {};
}

unsigned char Logger::m_outputFlags = static_cast<unsigned char>(Logger::OutputFlags::Console);
std::mutex Logger::m_mutex;

void Logger::ToLog(Severity severity, std::string const & message)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  ToLogImpl(SeverityToString(severity), ": " /* endLine */);
  ToLogImpl(message, "\n" /* endLine */);
}

void Logger::ToLogImpl(std::string const & message, std::string const & endLine)
{
  if ((m_outputFlags & static_cast<unsigned char>(OutputFlags::Console)) != 0)
  {
    std::cout << message;
    if (!endLine.empty())
      std::cout << endLine;
#ifdef WINDOWS_PLATFORM
    OutputDebugStringA((SeverityToString(severity) + message).c_str());
    if (!endLine.empty())
      OutputDebugStringA(endLine.c_str());
#endif
  }
  if ((m_outputFlags & static_cast<unsigned char>(OutputFlags::File)) != 0)
  {
    if (g_logFile.is_open())
    {
      g_logFile << message;
      if (!endLine.empty())
        g_logFile << endLine;
    }
  }
}

void Logger::ToLogWithFormat(Severity severity, char const * format, ...)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  ToLogImpl(SeverityToString(severity), ": " /* endLine */);
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

    ToLogImpl(buf, "\n" /* endLine */);
  }
  else
  {
    ToLogImpl(format, "\n" /* endLine */);
  }
}

void Logger::SetOutputFlags(unsigned char flags)
{
  m_outputFlags |= flags;
}

void Logger::Start(unsigned char flags)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  SetOutputFlags(flags);
  if ((m_outputFlags & (unsigned char)OutputFlags::File) != 0)
    g_logFile.open(kLogFileName);
}

void Logger::Finish()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (g_logFile.is_open())
  {
    g_logFile.flush();
    g_logFile.close();
  }
}

void Logger::Flush()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (g_logFile.is_open())
    g_logFile.flush();
}
}  // namespace rf
