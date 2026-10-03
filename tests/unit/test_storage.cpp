#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nexus_storage.h"
#include "nexus_storage_host.h"
#include "nexus_errors.h"

static char g_root[256];

static void make_root(void) {
    // Use a unique temp dir per test run.
    const char* base = getenv("TEMP");
    if (!base) base = getenv("TMP");
    if (!base) base = ".";
    snprintf(g_root, sizeof(g_root), "%s/nexus_storage_test", base);
}

static int setup(void) {
    make_root();
    nexus_storage_host_set_root(g_root);
    nexus_storage_host_inject_full(false);
    nexus_storage_host_inject_write_failure(false);
    nexus_storage_host_inject_mount_failure(false);

    nexus_storage_init();
    nexus_storage_select(nexus_storage_host_backend());
    // Start from a clean state for each test group.
    nexus_storage_unmount();
    return 0;
}

static int test_path_safety(void) {
    if (!nexus_storage_path_is_safe("/NEXUS/LOGS/x.log")) { printf("  FAIL: valid path rejected\n"); return 1; }
    if (nexus_storage_path_is_safe("NEXUS/LOGS/x.log")) { printf("  FAIL: relative accepted\n"); return 1; }
    if (nexus_storage_path_is_safe("/NEXUS/../secret")) { printf("  FAIL: traversal accepted\n"); return 1; }
    if (nexus_storage_path_is_safe("/NEXUS/LOGS/../../etc")) { printf("  FAIL: traversal accepted (2)\n"); return 1; }
    if (nexus_storage_path_is_safe("/NEXUS\\LOGS\\x")) { printf("  FAIL: backslash accepted\n"); return 1; }
    if (nexus_storage_path_is_safe("/NEXUS//x")) { printf("  FAIL: empty segment accepted\n"); return 1; }
    if (nexus_storage_path_is_safe("/NEXUS/LOGS/")) { printf("  FAIL: trailing slash accepted\n"); return 1; }

    char out[64];
    if (nexus_storage_sanitize_name("a/b:c*?.txt", out, sizeof(out)) != NEXUS_OK) { printf("  FAIL: sanitize returned err\n"); return 1; }
    if (strchr(out, '/') || strchr(out, ':') || strchr(out, '*')) { printf("  FAIL: sanitize left unsafe chars: '%s'\n", out); return 1; }
    printf("  PASS: path safety + sanitization\n");
    return 0;
}

static int test_mount_and_layout(void) {
    if (nexus_storage_mount() != NEXUS_OK) { printf("  FAIL: mount\n"); return 1; }
    if (!nexus_storage_is_ready()) { printf("  FAIL: not ready\n"); return 1; }
    if (nexus_storage_state() != NEXUS_STORAGE_READY) { printf("  FAIL: state\n"); return 1; }

    // Standard layout directories must exist after mount.
    static const char* dirs[] = {
        "/NEXUS", "/NEXUS/CONFIG", "/NEXUS/DEVICES", "/NEXUS/TESTS",
        "/NEXUS/REPORTS", "/NEXUS/CAPTURES", "/NEXUS/LOGS",
        "/NEXUS/CRASHES", "/NEXUS/EXPORTS", "/NEXUS/FIRMWARE",
    };
    for (size_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++) {
        if (!nexus_storage_exists(dirs[i])) { printf("  FAIL: missing dir %s\n", dirs[i]); return 1; }
    }
    printf("  PASS: mount + layout creation\n");

    nexus_storage_info_t info;
    if (nexus_storage_get_info(&info) != NEXUS_OK) { printf("  FAIL: info\n"); return 1; }
    if (info.total_bytes == 0) { printf("  FAIL: zero capacity\n"); return 1; }
    printf("  PASS: storage info\n");
    return 0;
}

static int test_read_write(void) {
    const char* path = "/NEXUS/LOGS/session.log";
    const char* text = "2026-10-03T01:23:42.381Z [INFO] SYSTEM Boot complete";
    if (nexus_storage_write_file(path, (const uint8_t*)text, strlen(text)) != NEXUS_OK) {
        printf("  FAIL: write\n"); return 1;
    }
    uint8_t buf[128];
    size_t n = 0;
    if (nexus_storage_read_file(path, buf, sizeof(buf), &n) != NEXUS_OK) { printf("  FAIL: read\n"); return 1; }
    if (n != strlen(text) || memcmp(buf, text, n) != 0) { printf("  FAIL: read mismatch\n"); return 1; }
    printf("  PASS: read/write\n");

    // Append
    const char* more = "\nsecond line";
    if (nexus_storage_append_file(path, (const uint8_t*)more, strlen(more)) != NEXUS_OK) { printf("  FAIL: append\n"); return 1; }
    if (nexus_storage_read_file(path, buf, sizeof(buf), &n) != NEXUS_OK) { printf("  FAIL: read after append\n"); return 1; }
    if (n != strlen(text) + strlen(more)) { printf("  FAIL: append size %zu\n", n); return 1; }
    printf("  PASS: append\n");

    // Exists / remove
    if (!nexus_storage_exists(path)) { printf("  FAIL: exists\n"); return 1; }
    if (nexus_storage_remove(path) != NEXUS_OK) { printf("  FAIL: remove\n"); return 1; }
    if (nexus_storage_exists(path)) { printf("  FAIL: still exists\n"); return 1; }
    printf("  PASS: exists/remove\n");
    return 0;
}

static int test_atomic_write(void) {
    // Use a missing intermediate dir? No - EXPORTS exists. Verify .tmp is
    // renamed away on success.
    const char* path = "/NEXUS/REPORTS/TEST_20261003_012530.json";
    const char* json = "{\"format\":\"nexus-test-report\",\"version\":1}";
    if (nexus_storage_write_atomic(path, (const uint8_t*)json, strlen(json)) != NEXUS_OK) {
        printf("  FAIL: atomic write\n"); return 1;
    }
    if (!nexus_storage_exists(path)) { printf("  FAIL: final file missing\n"); return 1; }

    // The temp file must be gone (renamed to final).
    char tmp[NEXUS_STORAGE_PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (nexus_storage_exists(tmp)) { printf("  FAIL: .tmp left behind\n"); return 1; }
    printf("  PASS: atomic .tmp -> final\n");
    return 0;
}

static int test_temp_recovery(void) {
    // Simulate an interrupted write: a stray .tmp in CONFIG.
    const char* tmp = "/NEXUS/CONFIG/system.json.tmp";
    const char* partial = "{\"partial\":";
    nexus_storage_write_file(tmp, (const uint8_t*)partial, strlen(partial));
    if (!nexus_storage_exists(tmp)) { printf("  FAIL: setup tmp\n"); return 1; }

    // Recovery runs during mount in the real flow.
    nexus_storage_recover_temp_files();
    if (nexus_storage_exists(tmp)) { printf("  FAIL: tmp not recovered\n"); return 1; }
    printf("  PASS: temp-file recovery\n");
    return 0;
}

static int test_list(void) {
    nexus_storage_write_file("/NEXUS/TESTS/a.json", (const uint8_t*)"{}", 2);
    nexus_storage_write_file("/NEXUS/TESTS/b.json", (const uint8_t*)"{}", 2);

    nexus_file_entry_t entries[16];
    uint32_t count = 0;
    if (nexus_storage_list("/NEXUS/TESTS", entries, 16, &count) != NEXUS_OK) { printf("  FAIL: list\n"); return 1; }
    if (count < 2) { printf("  FAIL: list count=%u\n", count); return 1; }
    printf("  PASS: list (%u entries)\n", count);
    return 0;
}

static int test_fault_injection(void) {
    // Write failure: operations must fail cleanly, system stays usable.
    nexus_storage_host_inject_write_failure(true);
    nexus_err_t e = nexus_storage_write_file("/NEXUS/LOGS/fail.log", (const uint8_t*)"x", 1);
    if (e == NEXUS_OK) { printf("  FAIL: write failure not detected\n"); return 1; }
    nexus_storage_host_inject_write_failure(false);

    // Card full: same contract.
    nexus_storage_host_inject_full(true);
    e = nexus_storage_write_file("/NEXUS/LOGS/full.log", (const uint8_t*)"x", 1);
    if (e == NEXUS_OK) { printf("  FAIL: full not detected\n"); return 1; }
    nexus_storage_host_inject_full(false);

    // After clearing, writes succeed again.
    if (nexus_storage_write_file("/NEXUS/LOGS/ok.log", (const uint8_t*)"x", 1) != NEXUS_OK) {
        printf("  FAIL: recovery write\n"); return 1;
    }
    printf("  PASS: fault injection (write-fail, full, recovery)\n");
    return 0;
}

static int test_missing_card(void) {
    // Unmount then attempt operations - must fail safely, not crash.
    nexus_storage_unmount();
    if (nexus_storage_is_ready()) { printf("  FAIL: ready after unmount\n"); return 1; }
    if (nexus_storage_exists("/NEXUS/LOGS/x.log")) { printf("  FAIL: exists on unmounted\n"); return 1; }
    if (nexus_storage_write_file("/NEXUS/LOGS/x.log", (const uint8_t*)"x", 1) == NEXUS_OK) {
        printf("  FAIL: write on unmounted\n"); return 1;
    }
    printf("  PASS: missing-card safety\n");
    return 0;
}

static int test_mount_failure(void) {
    nexus_storage_host_inject_mount_failure(true);
    if (nexus_storage_mount() == NEXUS_OK) { printf("  FAIL: mount failure not detected\n"); return 1; }
    if (nexus_storage_is_ready()) { printf("  FAIL: ready after failed mount\n"); return 1; }
    nexus_storage_host_inject_mount_failure(false);
    printf("  PASS: mount-failure handling\n");
    return 0;
}

int test_storage(void) {
    printf("\n");
    setup();
    if (test_path_safety()) return 1;
    if (test_mount_and_layout()) return 1;
    if (test_read_write()) return 1;
    if (test_atomic_write()) return 1;
    if (test_temp_recovery()) return 1;
    if (test_list()) return 1;
    if (test_fault_injection()) return 1;
    if (test_missing_card()) return 1;
    if (test_mount_failure()) return 1;
    return 0;
}
