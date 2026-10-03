#include "nexus_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static nexus_log_level_t s_log_level = NEXUS_LOG_INFO;
static uint32_t s_log_outputs = NEXUS_LOG_OUTPUT_SERIAL;
static nexus_log_sink_fn s_sink = NULL;

// Fixed buffer: no dynamic allocation, bounded output.
#define NEXUS_LOG_MSG_MAX 192

void nexus_log_init(void) {
    s_log_level = NEXUS_LOG_INFO;
    s_log_outputs = NEXUS_LOG_OUTPUT_SERIAL;
    s_sink = NULL;
}

void nexus_log_set_level(nexus_log_level_t level) {
    s_log_level = level;
}

nexus_log_level_t nexus_log_get_level(void) {
    return s_log_level;
}

void nexus_log_set_output(uint32_t outputs) {
    s_log_outputs = outputs;
}

void nexus_log_set_sink(nexus_log_sink_fn sink) {
    s_sink = sink;
}

const char* nexus_log_level_name(nexus_log_level_t level) {
    static const char* names[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    if (level < 0 || level > NEXUS_LOG_FATAL) return "?";
    return names[level];
}

void nexus_log_write(nexus_log_level_t level, const char* module, const char* fmt, ...) {
    if (level < s_log_level) return;
    if (!module) module = "?";
    if (!fmt) return;

    // Format the message once into a bounded buffer.
    char message[NEXUS_LOG_MSG_MAX];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    // Serial output.
    if (s_log_outputs & NEXUS_LOG_OUTPUT_SERIAL) {
        printf("[%s] %-12s %s\n", nexus_log_level_name(level), module, message);
        fflush(stdout);
    }

    // Forward every record to the structured sink (if registered). The sink is
    // independent of the serial bitmask: it is the machine-readable
    // destination, not a console mirror. It must not block.
    if (s_sink) {
        s_sink(level, module, message);
    }
}
