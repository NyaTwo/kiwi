// system.hpp

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <string_view>

using int64 = signed long long;

namespace debug 
{
   void info(const char *format, ...);
   void warn(const char *format, ...);
   void error(const char *format, ...);
} // !debug

struct mouse_moved_t     { int x, y; };
struct key_pressed_t     { int keycode; };
struct key_released_t    { int keycode; };
struct button_pressed_t  { int button; };
struct button_released_t { int button; };

class event_dispatcher_t {
public:
   event_dispatcher_t() = delete;

   template <typename T>
   static void dispatch(const T &event)
   {
      auto &listeners = get_listeners<T>();
      for (auto &listener : listeners) {
         listener(event);
      }
   }

   template <typename T, typename F>
   static void add_listener(F &listener)
   {
      auto &listeners = get_listeners<T>();
      listeners.emplace_back([&](const T &event) { listener.on_event(event); });
   }

private:
   template <typename T>
   static auto &get_listeners()
   {
      static std::vector<std::function<void(const T &)>> ms_listeners;
      return ms_listeners;
   }
};

struct file_system_t {
   file_system_t() = delete;

   static bool load_content(const std::string_view &filename, std::string &content);
   static bool load_content(const std::string_view &filename, std::vector<uint8_t> &content);
};

struct timespan_t {
   static constexpr timespan_t from_seconds(double value)      { return timespan_t{ int64(value * 1000000.0) }; }
   static constexpr timespan_t from_milliseconds(double value) { return timespan_t{ int64(value * 1000.0) }; }

   constexpr timespan_t() = default;
   constexpr timespan_t(const int64 duration) : m_duration(duration) {}

   bool operator==(const timespan_t &rhs) const;
   bool operator!=(const timespan_t &rhs) const;
   bool operator< (const timespan_t &rhs) const;
   bool operator<=(const timespan_t &rhs) const;
   bool operator> (const timespan_t &rhs) const;
   bool operator>=(const timespan_t &rhs) const;

   timespan_t  operator+ (const timespan_t &rhs) const;
   timespan_t  operator- (const timespan_t &rhs) const;
   timespan_t  operator* (const float rhs) const;
   timespan_t  operator/ (const float rhs) const;
   timespan_t &operator+=(const timespan_t &rhs);
   timespan_t &operator-=(const timespan_t &rhs);
   timespan_t &operator*=(const float rhs);
   timespan_t &operator/=(const float rhs);

   float elapsed_seconds() const;
   float elapsed_milliseonds() const;

   int64 m_duration = 0;
};

struct watch_t {
   watch_t() = delete;

   static timespan_t time_since_start();
};
