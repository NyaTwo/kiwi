// system.cpp

#include "system.hpp"
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdarg.h>

namespace debug 
{
   void info(const char *format, ...)
   {
      char message[2048];
      va_list args;
      va_start(args, format);
      vsprintf_s(message, format, args);
      va_end(args);
      printf("NFO: %s\n", message);
   }

   void warn(const char *format, ...)
   {
      char message[2048];
      va_list args;
      va_start(args, format);
      vsprintf_s(message, format, args);
      va_end(args);
      printf("WRN: %s\n", message);
   }

   void error(const char *format, ...) 
   {
      char message[2048];
      va_list args;
      va_start(args, format);
      vsprintf_s(message, format, args);
      va_end(args);
      printf("ERR: %s\n", message);
   }
} // !debug

template <typename T> 
static bool 
load_file_content(const std::string_view &filename, T &content)
{
   FILE *file = nullptr;
   fopen_s(&file, filename.data(), "rb");
   if (file == nullptr) {
      debug::warn("could not locate '%s'", filename.data());
      return false;
   }

   fseek(file, 0, SEEK_END);
   content.resize(ftell(file));
   fseek(file, 0, SEEK_SET);
   fread(content.data(), 1, content.size(), file);
   fclose(file);

   return true;
}

// static 
bool file_system_t::load_content(const std::string_view &filename, std::string &content)
{
   return load_file_content(filename, content);
}

bool file_system_t::load_content(const std::string_view &filename, std::vector<uint8_t> &content)
{
   return load_file_content(filename, content);
}

bool timespan_t::operator==(const timespan_t &rhs) const
{
   return m_duration == rhs.m_duration;
}

bool timespan_t::operator!=(const timespan_t &rhs) const
{
   return m_duration != rhs.m_duration;
}

bool timespan_t::operator< (const timespan_t &rhs) const
{
   return m_duration < rhs.m_duration;
}

bool timespan_t::operator<=(const timespan_t &rhs) const
{
   return m_duration <= rhs.m_duration;
}

bool timespan_t::operator> (const timespan_t &rhs) const
{
   return m_duration > rhs.m_duration;
}

bool timespan_t::operator>=(const timespan_t &rhs) const
{
   return m_duration >= rhs.m_duration;
}

timespan_t timespan_t::operator+ (const timespan_t &rhs) const
{
   return { m_duration + rhs.m_duration };
}

timespan_t timespan_t::operator- (const timespan_t &rhs) const
{
   return { m_duration - rhs.m_duration };
}

timespan_t timespan_t::operator* (const float rhs) const
{
   return { int64(m_duration * rhs) };
}

timespan_t timespan_t::operator/ (const float rhs) const
{
   return { int64(m_duration / rhs) };
}

timespan_t &timespan_t::operator+=(const timespan_t &rhs)
{
   m_duration += rhs.m_duration;
   return *this;
}

timespan_t &timespan_t::operator-=(const timespan_t &rhs)
{
   m_duration -= rhs.m_duration;
   return *this;
}

timespan_t &timespan_t::operator*=(const float rhs)
{
   m_duration = int64(m_duration * rhs);
   return *this;
}

timespan_t &timespan_t::operator/=(const float rhs)
{
   m_duration = int64(m_duration / rhs);
   return *this;
}

float timespan_t::elapsed_seconds() const
{
   return float(m_duration / 1000000.0);
}

float timespan_t::elapsed_milliseonds() const
{
   return float(m_duration / 1000.0);
}

// static 
timespan_t watch_t::time_since_start()
{
   const double now = glfwGetTime();
   return timespan_t{ int64(now * 1000000.0) };
}
