#ifndef NEXUS_LOG_H
#define NEXUS_LOG_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    NEXUS_LOG_TRACE = 0,
    NEXUS_LOG_DEBUG,
    NEXUS_LOG_INFO,
    NEXUS_LOG_WARN,
    NEXUS_LOG_ERROR,
    NEXUS_LOG_FATAL,
} nexus_log_level_t;

typedef enum {
    NEXUS_LOG_OUTPUT_NONE = 0,
    NEXUS_LOG_OUTPUT_SERIAL = (1 << 0),
    NEXUS_LOG_OUTPUT_SD = (1 << 1),
    NEXUS_LOG_OUTPUT_MEMORY = (1 << 2),
} nexus_log_output_t;

/*
 * A sink receives already-formatted log records. This is how the persistent
 * logger (nexus_logger) subscribes to every NEXUS_LOG_* call without each call
 * site needing to know about storage. The sink must be fast and non-blocking;
 * nexus_logger only enqueues.
 */
typedef void (*nexus_log_sink_fn)(nexus_log_level_t level, const char* module, const char* message);

void nexus_log_init(void);
void nexus_log_set_level(nexus_log_level_t level);
nexus_log_level_t nexus_log_get_level(void);
void nexus_log_set_output(uint32_t outputs);
void nexus_log_set_sink(nexus_log_sink_fn sink);
void nexus_log_write(nexus_log_level_t level, const char* module, const char* fmt, ...);

const char* nexus_log_level_name(nexus_log_level_t level);

#define NEXUS_LOG_TRACE(mod, ...) nexus_log_write(NEXUS_LOG_TRACE, mod, __VA_ARGS__)
#define NEXUS_LOG_DEBUG(mod, ...) nexus_log_write(NEXUS_LOG_DEBUG, mod, __VA_ARGS__)
#define NEXUS_LOG_INFO(mod, ...)  nexus_log_write(NEXUS_LOG_INFO,  mod, __VA_ARGS__)
#define NEXUS_LOG_WARN(mod, ...)  nexus_log_write(NEXUS_LOG_WARN,  mod, __VA_ARGS__)
#define NEXUS_LOG_ERROR(mod, ...) nexus_log_write(NEXUS_LOG_ERROR, mod, __VA_ARGS__)
#define NEXUS_LOG_FATAL(mod, ...) nexus_log_write(NEXUS_LOG_FATAL, mod, __VA_ARGS__)

#endif
