#ifndef NEXUS_PERSIST_H
#define NEXUS_PERSIST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

/*
 * Persistence of captures and test reports.
 *
 * Both formats carry a version so a future firmware can detect incompatible
 * files instead of misinterpreting them.
 *
 * Capture layout (directory per capture):
 *   /NEXUS/CAPTURES/CAP_<stamp>/
 *       meta.json    nexus-capture v1 metadata
 *       data.bin     raw sample payload
 *
 * Test report:
 *   /NEXUS/REPORTS/TEST_<stamp>.json
 *   /NEXUS/REPORTS/TEST_<stamp>.txt
 */

#define NEXUS_CAPTURE_FORMAT   "nexus-capture"
#define NEXUS_CAPTURE_VERSION  1
#define NEXUS_REPORT_FORMAT    "nexus-test-report"
#define NEXUS_REPORT_VERSION   1

typedef struct {
    char stamp[24];        // YYYYMMDD_HHMMSS
    char source[32];       // e.g. "UART3"
    char protocol[16];     // e.g. "UART"
    uint32_t sample_rate;  // Hz
    uint32_t baud;         // for UART
    char trigger[24];      // e.g. "RX"
    uint32_t sample_count;
    uint32_t duration_ms;
    bool wall_valid;
} nexus_capture_meta_t;

// Writes meta.json + data.bin atomically-ish. Returns the capture dir path.
nexus_err_t nexus_persist_capture(const nexus_capture_meta_t* meta,
                                  const uint8_t* data, size_t data_len,
                                  char* out_dir, size_t out_dir_size);

// Serialize the capture metadata to JSON (exposed for tests).
nexus_err_t nexus_persist_capture_meta_json(const nexus_capture_meta_t* meta,
                                            uint32_t data_len,
                                            char* out, size_t out_size);

// Write a test report as JSON + TXT.
nexus_err_t nexus_persist_test_report(const char* test_name,
                                      int result, // matches nexus_test_result_t
                                      uint32_t duration_ms,
                                      uint32_t steps_passed,
                                      uint32_t steps_failed,
                                      uint32_t steps_skipped,
                                      uint32_t steps_total,
                                      char* out_json, size_t out_json_size,
                                      char* out_txt, size_t out_txt_size);

#endif
