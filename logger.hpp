#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <cstdint>
#include <mutex>
#include <sstream>
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

  template<typename... Args>
  static void ToLog(Severity severity, Args && ... args)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    ToLogImpl(SeverityToString(severity), ": " /* endLine */);

    int cnt = 0;
    (Count(args, cnt), ...);

    std::string const kDelimiter = " ";
    (ToLogImpl(args, --cnt ? kDelimiter : "" /* endLine */), ...);
    ToLogImpl("\n", "" /* endLine */);
  }

private:
  template <typename T> static void Count(T const & val, int & counter) { counter++; }

  static void ToLogImpl(std::string const & message, std::string const & endLine);

  template <typename T> static void ToLogImpl(T val, std::string const & endLine)
  {
    std::ostringstream ss;
    ss << val;
    ToLogImpl(ss.str(), endLine);
  }

  static void SetOutputFlags(unsigned char flags);

  static unsigned char m_outputFlags;
  static std::mutex m_mutex;

  friend std::string SeverityToString(Logger::Severity s);
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

namespace glm
{
inline std::ostream & operator<<(std::ostream & os, glm::vec2 const & v)
{
  os << glm::to_string(v);
  return os;
}

inline std::ostream & operator<<(std::ostream & os, glm::vec3 const & v)
{
  os << glm::to_string(v);
  return os;
}

inline std::ostream & operator<<(std::ostream & os, glm::vec4 const & v)
{
  os << glm::to_string(v);
  return os;
}

inline std::ostream & operator<<(std::ostream & os, glm::mat4x4 const & m)
{
  os << glm::to_string(m);
  return os;
}
}  // namespace glm
