#include "nexus_persist.h"
#include "nexus_storage.h"
#include "nexus_time.h"
#include "nexus_version.h"
#include <string.h>
#include <stdio.h>

static void current_stamp(char* out, size_t out_size) {
    nexus_datetime_t dt;
    if (nexus_time_is_valid() && nexus_time_now(&dt) == NEXUS_OK) {
        nexus_time_format_stamp(&dt, out, out_size);
    } else {
        // No wall clock: use a monotonic-ish fallback name that is still unique
        // per boot by embedding the millisecond counter would require a clock
        // here; instead use a fixed marker so files are clearly untimestamped.
        snprintf(out, out_size, "NO_TIME");
    }
}

nexus_err_t nexus_persist_capture_meta_json(const nexus_capture_meta_t* meta,
                                            uint32_t data_len,
                                            char* out, size_t out_size) {
    if (!meta || !out || out_size == 0) return NEXUS_ERR_INVALID_PARAM;

    char iso[32] = "null";
    if (meta->wall_valid && nexus_time_is_valid()) {
        nexus_datetime_t dt;
        nexus_time_now(&dt);
        nexus_time_format_iso8601(&dt, iso, sizeof(iso));
    }

    snprintf(out, out_size,
        "{\n"
        "  \"format\": \"%s\",\n"
        "  \"version\": %d,\n"
        "  \"timestamp\": %s,\n"
        "  \"source\": \"%s\",\n"
        "  \"protocol\": \"%s\",\n"
        "  \"baud\": %u,\n"
        "  \"sample_rate\": %u,\n"
        "  \"trigger\": \"%s\",\n"
        "  \"sample_count\": %u,\n"
        "  \"duration_ms\": %u,\n"
        "  \"data_file\": \"data.bin\",\n"
        "  \"data_bytes\": %u\n"
        "}\n",
        NEXUS_CAPTURE_FORMAT, NEXUS_CAPTURE_VERSION, iso,
        meta->source, meta->protocol, meta->baud, meta->sample_rate,
        meta->trigger, meta->sample_count, meta->duration_ms, data_len);
    return NEXUS_OK;
}

nexus_err_t nexus_persist_capture(const nexus_capture_meta_t* meta,
                                  const uint8_t* data, size_t data_len,
                                  char* out_dir, size_t out_dir_size) {
    if (!meta || !data) return NEXUS_ERR_INVALID_PARAM;
    if (!nexus_storage_is_ready()) return NEXUS_ERR_STORAGE;

    char stamp[24];
    if (meta->stamp[0]) {
        strncpy(stamp, meta->stamp, sizeof(stamp) - 1);
        stamp[sizeof(stamp) - 1] = '\0';
    } else {
        current_stamp(stamp, sizeof(stamp));
    }

    char dir[NEXUS_STORAGE_PATH_MAX];
    snprintf(dir, sizeof(dir), "%s/CAPTURES/CAP_%s", NEXUS_STORAGE_ROOT, stamp);

    nexus_err_t err = nexus_storage_mkdir(dir);
    if (err != NEXUS_OK) return err;

    // Write data first, then metadata (metadata last = commit point).
    char data_path[NEXUS_STORAGE_PATH_MAX];
    snprintf(data_path, sizeof(data_path), "%s/data.bin", dir);
    err = nexus_storage_write_file(data_path, data, data_len);
    if (err != NEXUS_OK) return err;

    char meta_json[512];
    nexus_persist_capture_meta_json(meta, (uint32_t)data_len, meta_json, sizeof(meta_json));

    char meta_path[NEXUS_STORAGE_PATH_MAX];
    snprintf(meta_path, sizeof(meta_path), "%s/meta.json", dir);
    err = nexus_storage_write_atomic(meta_path, (const uint8_t*)meta_json, strlen(meta_json));
    if (err != NEXUS_OK) return err;

    if (out_dir && out_dir_size) {
        strncpy(out_dir, dir, out_dir_size - 1);
        out_dir[out_dir_size - 1] = '\0';
    }
    return NEXUS_OK;
}

nexus_err_t nexus_persist_test_report(const char* test_name,
                                      int result,
                                      uint32_t duration_ms,
                                      uint32_t steps_passed,
                                      uint32_t steps_failed,
                                      uint32_t steps_skipped,
                                      uint32_t steps_total,
                                      char* out_json, size_t out_json_size,
                                      char* out_txt, size_t out_txt_size) {
    if (!test_name) return NEXUS_ERR_INVALID_PARAM;
    if (!nexus_storage_is_ready()) return NEXUS_ERR_STORAGE;

    static const char* const results[] = {"PASS", "FAIL", "SKIPPED", "ERROR", "TIMEOUT"};
    const char* result_str = (result >= 0 && result <= 4) ? results[result] : "UNKNOWN";

    char iso[32] = "null";
    char stamp[24];
    current_stamp(stamp, sizeof(stamp));
    if (nexus_time_is_valid()) {
        nexus_datetime_t dt;
        nexus_time_now(&dt);
        nexus_time_format_iso8601(&dt, iso, sizeof(iso));
    }

    char json[768];
    snprintf(json, sizeof(json),
        "{\n"
        "  \"format\": \"%s\",\n"
        "  \"version\": %d,\n"
        "  \"firmware\": \"%s\",\n"
        "  \"test\": \"%s\",\n"
        "  \"timestamp\": %s,\n"
        "  \"result\": \"%s\",\n"
        "  \"duration_ms\": %u,\n"
        "  \"steps\": { \"passed\": %u, \"failed\": %u, \"skipped\": %u, \"total\": %u }\n"
        "}\n",
        NEXUS_REPORT_FORMAT, NEXUS_REPORT_VERSION, NEXUS_VERSION_STRING, test_name, iso,
        result_str, duration_ms, steps_passed, steps_failed, steps_skipped, steps_total);

    char txt[512];
    snprintf(txt, sizeof(txt),
        "WattLab Nexus\n"
        "Automated DUT Test\n\n"
        "Test:     %s\n"
        "Result:   %s\n"
        "Duration: %u ms\n"
        "Steps:    %u passed, %u failed, %u skipped (%u total)\n"
        "Firmware: %s\n"
        "Timestamp:%s\n",
        test_name, result_str, duration_ms,
        steps_passed, steps_failed, steps_skipped, steps_total,
        NEXUS_VERSION_STRING, iso);

    char jpath[NEXUS_STORAGE_PATH_MAX];
    char tpath[NEXUS_STORAGE_PATH_MAX];
    snprintf(jpath, sizeof(jpath), "%s/REPORTS/TEST_%s.json", NEXUS_STORAGE_ROOT, stamp);
    snprintf(tpath, sizeof(tpath), "%s/REPORTS/TEST_%s.txt", NEXUS_STORAGE_ROOT, stamp);

    nexus_err_t err = nexus_storage_write_atomic(jpath, (const uint8_t*)json, strlen(json));
    if (err != NEXUS_OK) return err;
    err = nexus_storage_write_atomic(tpath, (const uint8_t*)txt, strlen(txt));
    if (err != NEXUS_OK) return err;

    if (out_json && out_json_size) {
        strncpy(out_json, jpath, out_json_size - 1);
        out_json[out_json_size - 1] = '\0';
    }
    if (out_txt && out_txt_size) {
        strncpy(out_txt, tpath, out_txt_size - 1);
        out_txt[out_txt_size - 1] = '\0';
    }
    return NEXUS_OK;
}
