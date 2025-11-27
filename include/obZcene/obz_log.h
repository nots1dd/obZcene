#ifndef OBZ_LOG_H
#define OBZ_LOG_H

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum
{
  OBZ_LOG_TRACE = 0,
  OBZ_LOG_DEBUG,
  OBZ_LOG_INFO,
  OBZ_LOG_WARN,
  OBZ_LOG_ERROR,
  OBZ_LOG_FATAL
} OBZ_LogLevel;

typedef struct Logger
{
  FILE*           log_file;  // Optional file to write logs
  OBZ_LogLevel    level;     // Minimum log level to write
  pthread_mutex_t lock;      // Thread safety
  int             use_color; // Whether to use colors for stdout
} OBZ_Logger;

#define OBZ_COLOR_RESET_ANSI "\033[0m"
#define OBZ_COLOR_TRACE_ANSI "\033[37m" // White
#define OBZ_COLOR_DEBUG_ANSI "\033[36m" // Cyan
#define OBZ_COLOR_INFO_ANSI  "\033[32m" // Green
#define OBZ_COLOR_WARN_ANSI  "\033[33m" // Yellow
#define OBZ_COLOR_ERROR_ANSI "\033[31m" // Red
#define OBZ_COLOR_FATAL_ANSI "\033[41m" // Red background

// not really working will fix this
extern OBZ_Logger* OBZ_GLOBAL_LOGGER;

#define DECLARE_OBZ_GLOBAL_LOGGER() OBZ_Logger* OBZ_GLOBAL_LOGGER = NULL;

// Set the global logger
static inline void OBZ_set_global_logger(OBZ_Logger* logger) { OBZ_GLOBAL_LOGGER = logger; }

// Get the global logger
static inline OBZ_Logger* OBZ_get_global_logger(void) { return OBZ_GLOBAL_LOGGER; }

static inline OBZ_Logger* OBZ_logger_init(const char* file_path, OBZ_LogLevel level)
{
  OBZ_Logger* logger = (OBZ_Logger*)malloc(sizeof(OBZ_Logger));
  if (!logger)
    return NULL;

  logger->level = level;
  pthread_mutex_init(&logger->lock, NULL);

  if (file_path)
  {
    logger->log_file = fopen(file_path, "a");
    if (!logger->log_file)
    {
      fprintf(stderr, "Logger init error: %s\n", strerror(errno));
      free(logger);
      return NULL;
    }
  }
  else
  {
    logger->log_file = NULL;
  }

  logger->use_color = 1; // Always use color on stdout
  return logger;
}

static inline void OBZ_logger_destroy(OBZ_Logger* logger)
{
  if (!logger)
    return;
  if (logger->log_file)
    fclose(logger->log_file);
  pthread_mutex_destroy(&logger->lock);
  free(logger);
}

// ----------------------------
// Internal log function
// ----------------------------
static inline void __OBZ_logger_log(OBZ_Logger* logger, OBZ_LogLevel level, const char* file,
                                    int line, const char* fmt, ...)
{
  if (!logger || level < logger->level)
    return;

  static const char* level_names[]  = {"TRAC", "DBUG", "INFO", "WARN", "ERRO", "FATL"};
  static const char* level_colors[] = {OBZ_COLOR_TRACE_ANSI, OBZ_COLOR_DEBUG_ANSI,
                                       OBZ_COLOR_INFO_ANSI,  OBZ_COLOR_WARN_ANSI,
                                       OBZ_COLOR_ERROR_ANSI, OBZ_COLOR_FATAL_ANSI};

  time_t    t = time(NULL);
  struct tm tm_info;
  localtime_r(&t, &tm_info);

  char time_buf[20];
  strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);

  va_list args;

  pthread_mutex_lock(&logger->lock);

  // Print to stdout with color
  if (logger->use_color)
  {
    fprintf(stdout, "[%s] [%s%s%s] [%s:%d]:  ", time_buf, level_colors[level], level_names[level],
            OBZ_COLOR_RESET_ANSI, file, line);
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fprintf(stdout, "\n");
    fflush(stdout);
  }

  // Print to file without color
  if (logger->log_file)
  {
    fprintf(logger->log_file, "[%s] [%s] [%s:%d]: ", time_buf, level_names[level], file, line);
    va_start(args, fmt);
    vfprintf(logger->log_file, fmt, args);
    va_end(args);
    fprintf(logger->log_file, "\n");
    fflush(logger->log_file);
  }

  pthread_mutex_unlock(&logger->lock);
}

// ----------------------------
// Convenience macros using explicit or global logger
// ----------------------------
//
// !!ONLY USE THESE MACROS!!
#define OBZ_LOG_TRACE(logger, fmt, ...)                                                    \
  __OBZ_logger_log((logger) ? (logger) : OBZ_get_global_logger(), OBZ_LOG_TRACE, __FILE__, \
                   __LINE__, fmt, ##__VA_ARGS__)
#define OBZ_LOG_DEBUG(logger, fmt, ...)                                                    \
  __OBZ_logger_log((logger) ? (logger) : OBZ_get_global_logger(), OBZ_LOG_DEBUG, __FILE__, \
                   __LINE__, fmt, ##__VA_ARGS__)
#define OBZ_LOG_INFO(logger, fmt, ...)                                                    \
  __OBZ_logger_log((logger) ? (logger) : OBZ_get_global_logger(), OBZ_LOG_INFO, __FILE__, \
                   __LINE__, fmt, ##__VA_ARGS__)
#define OBZ_LOG_WARN(logger, fmt, ...)                                                    \
  __OBZ_logger_log((logger) ? (logger) : OBZ_get_global_logger(), OBZ_LOG_WARN, __FILE__, \
                   __LINE__, fmt, ##__VA_ARGS__)
#define OBZ_LOG_ERROR(logger, fmt, ...)                                                    \
  __OBZ_logger_log((logger) ? (logger) : OBZ_get_global_logger(), OBZ_LOG_ERROR, __FILE__, \
                   __LINE__, fmt, ##__VA_ARGS__)
#define OBZ_LOG_FATAL(logger, fmt, ...)                                                    \
  __OBZ_logger_log((logger) ? (logger) : OBZ_get_global_logger(), OBZ_LOG_FATAL, __FILE__, \
                   __LINE__, fmt, ##__VA_ARGS__)

#endif // OBZ_LOG_H
