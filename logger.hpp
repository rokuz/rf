#pragma once

#include <cstdint>
#include <string>

namespace rf
{
class Logger
{
public:
  enum OutputFlags : uint8_t
  {
    Console = 1 << 0,
    File = 1 << 1
  };

  enum Severity
  {
    Error,
    Warning,
    Info
  };

  static void Start(unsigned char flags = OutputFlags::Console);
  static void Finish();
  static void Flush();

  static void ToLog(Severity severity, std::string const & message);
  static void ToLogWithFormat(Severity severity, char const * format, ...);

private:
  static void ToLogImpl(Severity severity, std::string const & message);
  static void SetOutputFlags(unsigned char flags);

  static unsigned char outputFlags;
};

class LoggerGuard
{
public:
  explicit LoggerGuard(unsigned char flags = Logger::OutputFlags::Console)
  {
    Logger::Start(flags);
  }

  ~LoggerGuard()
  {
    Logger::Finish();
  }
};
}  // namespace rf
