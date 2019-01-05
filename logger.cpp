#include "logger.hpp"

#include <fstream>
#include <iostream>

namespace rf
{
char const * const kLogFileName = "log.txt";

std::ofstream g_logFile;
unsigned char Logger::outputFlags = static_cast<unsigned char>(Logger::OutputFlags::CONSOLE);

void Logger::ToLog(std::string const & message)
{
  if ((outputFlags & static_cast<unsigned char>(OutputFlags::CONSOLE)) != 0)
  {
    std::cout << message;
  }
  if ((outputFlags & static_cast<unsigned char>(OutputFlags::FILE)) != 0)
  {
    if (g_logFile.is_open())
      g_logFile << message;
  }
}

void Logger::ToLogWithFormat(char const * format, ...)
{
  std::string fmt(format);
  auto const n = std::count(fmt.begin(), fmt.end(), '%');
  if (n > 0)
  {
    static char buf[2048];
    va_list args;
    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);

    ToLog(buf);
  }
  else
  {
    ToLog(format);
  }
}

void Logger::SetOutputFlags(unsigned char flags)
{
  outputFlags |= flags;
}

void Logger::Start(unsigned char flags)
{
  SetOutputFlags(flags);
  if ((outputFlags & (unsigned char)OutputFlags::FILE) != 0)
    g_logFile.open(kLogFileName);
}

void Logger::Finish()
{
  if (g_logFile.is_open())
  {
    g_logFile.flush();
    g_logFile.close();
  }
}

void Logger::Flush()
{
  if (g_logFile.is_open())
    g_logFile.flush();
}
}  // namespace common