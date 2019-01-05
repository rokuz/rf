#pragma once

#include <string>

namespace rf
{
class Logger
{
public:
  enum OutputFlags
  {
    CONSOLE = 1 << 0,
    FILE = 1 << 1
  };

  static void Start(unsigned char flags = OutputFlags::CONSOLE);
  static void Finish();
  static void Flush();

  static void ToLog(std::string const & message);
  static void ToLogWithFormat(char const * format, ...);

private:
  static void SetOutputFlags(unsigned char flags);

  static unsigned char outputFlags;
};

class LoggerGuard
{
public:
  LoggerGuard(unsigned char flags = Logger::OutputFlags::CONSOLE)
  {
    Logger::Start(flags);
  }

  ~LoggerGuard()
  {
    Logger::Finish();
  }
};
}  // namespace rf
