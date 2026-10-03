#include <stdio.h>
#include <string.h>
#include "nexus_crash.h"
#include "nexus_errors.h"

int test_crash(void) {
    printf("\n");

    nexus_crash_init();

    // Initially no record
    if (nexus_crash_has_record()) { printf("  FAIL: unexpected record at boot\n"); return 1; }
    printf("  PASS: no crash record initially\n");

    // Record a crash
    nexus_crash_record("Capture", NEXUS_RESET_IWDG);
    if (!nexus_crash_has_record()) { printf("  FAIL: record not valid after write\n"); return 1; }
    printf("  PASS: crash record stored\n");

    const nexus_crash_record_t* r = nexus_crash_get_record();
    if (r == NULL) { printf("  FAIL: get record NULL\n"); return 1; }
    if (strcmp(r->task_name, "Capture") != 0) { printf("  FAIL: task name=%s\n", r->task_name); return 1; }
    if (r->reset_reason != NEXUS_RESET_IWDG) { printf("  FAIL: reset reason\n"); return 1; }
    printf("  PASS: crash record fields\n");

    // Tampering invalidates the checksum
    nexus_crash_record_t* mut = (nexus_crash_record_t*)r;
    uint32_t saved = mut->pc;
    mut->pc = 0xDEADBEEF;
    if (nexus_crash_has_record()) { printf("  FAIL: tampered record accepted\n"); return 1; }
    mut->pc = saved;
    printf("  PASS: checksum detects tampering\n");

    // Formatting
    char buf[256];
    nexus_err_t err = nexus_crash_format(buf, sizeof(buf));
    if (err != NEXUS_OK) { printf("  FAIL: format err=%d\n", err); return 1; }
    if (strstr(buf, "Capture") == NULL) { printf("  FAIL: format missing task\n"); return 1; }
    printf("  PASS: crash report formatting\n");

    // Clear
    nexus_crash_clear();
    if (nexus_crash_has_record()) { printf("  FAIL: record still present after clear\n"); return 1; }
    printf("  PASS: crash record cleared\n");

    return 0;
}
