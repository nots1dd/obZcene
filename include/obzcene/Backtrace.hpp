#pragma once

/**
 * @file Backtrace.hpp
 * @brief Debug-only call-stack tracing and event log (namespace `obzcene::core`).
 *
 * When built with `OBZCENE_DEBUG_BUILD`, `RECORD_FUNC_TO_BACKTRACE` marks
 * function entry/exit in an in-memory event log whose frames carry libbacktrace
 * stack captures; `DUMP_TRACE()` prints the log. In release builds both macros
 * compile to no-ops, so this header has zero production cost. It is a
 * development aid: prefer spdlog (obzcene::Logger) for normal logging.
 *
 * @warning Memory grows on every recorded event — do not leave debug builds
 *          running long.
 *
 * @see Core/Logger.hpp (production logging),
 *      utils/timer/Timer.hpp (timestamp_now used in Event)
 */

#include "obzcene/utils/Env-Vars.hpp"
#include "obzcene/utils/string/Transforms.hpp"
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

// Note:
//
// This header is not used in release build.
//
// StackTrace:
//
// Uses libbacktrace like so:
//
// 1. Record a function for backtrace
// (so in runtime we get the info that at this time, this function was called from this parent)
//
// 2. If it doesnt exit the functional scope, we know that something went wrong within that scope
// (or its children scopes)
//
// It is very useful for me to debug and understand the program flow of where things went wrong.
//
// IMPORTANT:
// Depending on where we record backtraces, memory usage keeps spiking over time quite fast!
//
// It is highly recommended NOT to use debug build for regular purposes!!
//

#ifdef OBZCENE_DEBUG_BUILD

#include "obzcene/utils/timer/Timer.hpp"
#include <backtrace-supported.h>
#include <backtrace.h>
#include <cstdlib>
#include <cxxabi.h>
#include <iostream>
#include <mutex>
#include <utility>

#endif

namespace obzcene::core
{

/**
 * @brief Whether stack-trace dumping is enabled via the environment.
 * @return True when `OBZCENE_STACK_TRACE_DUMP_ENV` is "1"/"true"/"yes"/"on"
 *         (case-insensitive); checked once on first call.
 */
inline auto is_stack_trace_enabled() -> bool
{
  static const bool enabled = []() -> bool
  {
    if (const char* v = std::getenv(OBZCENE_STACK_TRACE_DUMP_ENV))
    {
      const auto s = utils::string::transform::tolower_ascii(v);
      return s == "1" || s == "true" || s == "yes" || s == "on";
    }
    return false;
  }();
  return enabled;
}

/** @brief True when stdout is a terminal (used to decide on ANSI colours). */
inline auto use_color() -> bool { return isatty(fileno(stdout)); }

/** @brief Wrap `s` in the ANSI code `c`, unless output is not a terminal. */
inline auto color(const char* c, const std::string& s) -> std::string
{
  if (!use_color())
    return s;
  return std::string(c) + s + "\033[0m";
}

inline auto bold(const std::string& s) { return color("\033[1m", s); }
inline auto cyan(const std::string& s) { return color("\033[36m", s); }
inline auto green(const std::string& s) { return color("\033[32m", s); }
inline auto red(const std::string& s) { return color("\033[31m", s); }
inline auto dim(const std::string& s) { return color("\033[2m", s); }
inline auto yellow(const std::string& s) { return color("\033[33m", s); }

/**
 * @brief Trim a mangled symbol for readability: strips common prefixes and
 *        everything from the first template `<`.
 */
inline auto simplify_symbol(std::string s) -> std::string
{
  auto cut = [&](const std::string& tok) -> void
  {
    if (auto p = s.find(tok); p != std::string::npos)
      s.erase(p);
  };

  cut("std::__cxx11::");
  cut("std::");
  cut("obzcene::core::");
  cut("core::");
  cut("inline ");
  cut("static ");

  if (auto p = s.find('<'); p != std::string::npos)
    s.erase(p);

  return s;
}

/** @brief One captured stack frame (function + file + line). */
struct StackFrame
{
  std::string function;
  std::string file;
  int         line = 0;

  /**
   * @brief Render the frame with a tree-style prefix of `depth` connectors.
   */
  [[nodiscard]] auto to_string(size_t depth) const -> std::string
  {
    std::ostringstream oss;
    oss << "  ";
    for (size_t i = 0; i < depth; ++i)
      oss << "│ ";

    oss << cyan("→ ") << cyan(function) << "\n";
    oss << "  ";
    for (size_t i = 0; i < depth; ++i)
      oss << "│ ";

    oss << dim("  at ") << dim(file) << yellow(":" + std::to_string(line)) << "\n";
    return oss.str();
  }
};

/** @brief A call stack captured at one instant. */
struct StackTrace
{
  /** @brief Frames, innermost first. */
  std::vector<StackFrame> frames;

  /** @brief Render the whole trace as an indented tree. */
  [[nodiscard]] auto to_string() const -> std::string
  {
    std::ostringstream oss;
    for (size_t i = 0; i < frames.size(); ++i)
      oss << frames[i].to_string(i);
    return oss.str();
  }
};

#ifdef OBZCENE_DEBUG_BUILD

/**
 * @brief Collects a StackTrace via the libbacktrace unwinder.
 */
class BacktraceCollector
{
public:
  /**
   * @brief Capture the current call stack.
   * @param skip  Number of innermost frames to drop (the collector itself).
   * @return Demangled, simplified frames; empty when backtrace is unavailable.
   */
  static auto capture(int skip = 0) -> StackTrace
  {
    StackTrace              trace;
    static backtrace_state* state = backtrace_create_state(nullptr, 0, error_cb, nullptr);

    if (!state)
      return trace;

    backtrace_full(state, skip + 1, frame_cb, error_cb, &trace);
    return trace;
  }

private:
  static void error_cb(void*, const char*, int) {}

  static auto frame_cb(void* data, uintptr_t, const char* file, int line, const char* func) -> int
  {
    auto*      trace = static_cast<StackTrace*>(data);
    StackFrame f;
    f.file     = file ? file : "<unknown>";
    f.line     = line;
    f.function = simplify_symbol(demangle(func));
    trace->frames.emplace_back(std::move(f));
    return 0;
  }

  /** @brief Demangle a mangled name; returns the raw name on failure. */
  static auto demangle(const char* name) -> std::string
  {
    if (!name)
      return "<unknown>";
    int         st  = 0;
    size_t      len = 0;
    char*       out = abi::__cxa_demangle(name, nullptr, &len, &st);
    std::string r   = (st == 0 && out) ? out : name;
    std::free(out);
    return r;
  }
};

/**
 * @brief An event log entry: timestamp + category + payload + captured stack.
 * @tparam Payload  Arbitrary per-event data (defaults to `std::string`).
 */
template <typename Payload = std::string> struct Event
{
  std::string ts;
  std::string cat;
  Payload     data;
  StackTrace  trace;

  Event(std::string c, Payload p)
      : ts(utils::timer::timestamp_now()), cat(std::move(c)), data(std::move(p)),
        trace(BacktraceCollector::capture(3))
  {
  }

  /** @brief Render the event as a boxed summary plus its stack trace. */
  [[nodiscard]] auto summary(size_t index) const -> std::string
  {
    constexpr size_t width = 60;

    auto pad = [](std::string s) -> std::string
    {
      if (s.size() < width - 2)
        s += std::string(width - 2 - s.size(), ' ');
      return s;
    };

    std::ostringstream oss;

    std::string line1 = bold(ts) + "  " + cat;
    std::string line2 = data;

    oss << "┌" << std::string(width - 2, '-') << "┐\n";
    oss << "│ " << "\n";
    oss << "│ " << "#" << index << " " << pad(line1) << "\n";
    oss << "│ " << pad(line2) << "\n";
    oss << "│ " << "\n";
    oss << "└" << std::string(width - 2, '-') << "┘\n";

    oss << trace.to_string();
    return oss.str();
  }
};

/** @brief Container of recorded events. */
template <typename T> using Events = std::vector<Event<T>>;

/**
 * @brief Thread-safe in-memory event log (singleton via #instance).
 * @tparam Payload  Event payload type.
 */
template <typename Payload = std::string> class EventLog
{
public:
  /** @brief The process-wide log instance. */
  static auto instance() -> EventLog&
  {
    static EventLog inst;
    return inst;
  }

  /** @brief Append a logged event (with stack capture) under `cat`. */
  void record(const std::string& cat, const Payload& p)
  {
    std::lock_guard<std::mutex> l(m_mutex);
    m_payloadEvents.emplace_back(cat, p);
  }

  /** @brief Print every recorded event, newest last, to stdout. */
  void dump_to_stdout() const
  {
    std::lock_guard<std::mutex> l(m_mutex);
    size_t                      index = 0;
    for (const auto& e : m_payloadEvents)
    {
      std::cout << "\n" << e.summary(index++);
    }
  }

private:
  mutable std::mutex m_mutex;
  Events<Payload>    m_payloadEvents;
};

/**
 * @brief RAII entry/exit marker: logs `enter` on construction, `exit` on
 *        destruction.
 * @see RECORD_FUNC_TO_BACKTRACE for the convenience macro.
 */
class TraceScope
{
public:
  TraceScope(std::string cat, std::string fn) : m_cat(std::move(cat)), m_func(simplify_symbol(fn))
  {
    EventLog<std::string>::instance().record(m_cat, green("enter ") + m_func);
  }

  ~TraceScope() { EventLog<std::string>::instance().record(m_cat, red("exit  ") + m_func); }

private:
  std::string m_cat;
  std::string m_func;
};

#endif

} // namespace obzcene::core

/**
 * @brief Log the entry/exit of the enclosing function to the backtrace log.
 * @param cat  Event category tag.
 *
 * Uses `__PRETTY_FUNCTION__` (or __FUNCSIG__/__FUNCTION__ for MSVC), and is a
 * no-op unless built with OBZCENE_DEBUG_BUILD.
 */
#if defined(__GNUC__) || defined(__clang__)
#ifdef OBZCENE_DEBUG_BUILD
#define RECORD_FUNC_TO_BACKTRACE(cat) \
  obzcene::core::TraceScope trace_scope(cat, __PRETTY_FUNCTION__)
#else
// No-op in release builds
#define RECORD_FUNC_TO_BACKTRACE(cat) (void(0))
#endif
#elif defined(_MSC_VER)
#define RECORD_FUNC_TO_BACKTRACE(cat) obzcene::core::TraceScope trace_scope(cat, __FUNCSIG__)
#else
#define RECORD_FUNC_TO_BACKTRACE(cat) obzcene::core::TraceScope trace_scope(cat, __FUNCTION__)
#endif

/**
 * @brief Print the accumulated backtrace event log to stdout.
 *
 * In debug builds nothing (or a hint to enable dumping) is printed unless
 * `OBZCENE_STACK_TRACE_DUMP_ENV` is set; in release builds it always prints a
 * "trace disabled" note.
 */
#ifdef OBZCENE_DEBUG_BUILD
#define DUMP_TRACE()                                                                           \
  do                                                                                           \
  {                                                                                            \
    if (obzcene::core::is_stack_trace_enabled())                                               \
      obzcene::core::EventLog<std::string>::instance().dump_to_stdout();                       \
    else                                                                                       \
      std::cout << "[trace disabled] set " << OBZCENE_STACK_TRACE_DUMP_ENV << "=1 to dump.\n"; \
  } while (0)
#else
#define DUMP_TRACE()                                                      \
  do                                                                      \
  {                                                                       \
    std::cout << "[trace disabled] build without OBZCENE_DEBUG_BUILD.\n"; \
  } while (0)
#endif
