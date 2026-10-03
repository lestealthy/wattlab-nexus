#include <stdio.h>
#include <stdlib.h>

// Test function declarations
int test_config(void);
int test_capture(void);
int test_device_db(void);
int test_test_engine(void);
int test_resource(void);
int test_decoders(void);
int test_measurement(void);
int test_message(void);
int test_crash(void);
int test_time(void);
int test_storage(void);
int test_logger(void);
int test_integration(void);
int test_alpha_flow(void);

typedef struct {
    const char* name;
    int (*func)(void);
} test_entry_t;

static const test_entry_t s_tests[] = {
    {"config", test_config},
    {"capture", test_capture},
    {"device_db", test_device_db},
    {"test_engine", test_test_engine},
    {"resource", test_resource},
    {"decoders", test_decoders},
    {"measurement", test_measurement},
    {"message", test_message},
    {"crash", test_crash},
    {"time", test_time},
    {"storage", test_storage},
    {"logger", test_logger},
    {"integration", test_integration},
    {"alpha_flow", test_alpha_flow},
};

#define NUM_TESTS (sizeof(s_tests) / sizeof(s_tests[0]))

int main(void) {
    printf("========================================\n");
    printf("WattLab Nexus - Host Test Suite\n");
    printf("========================================\n\n");

    int passed = 0;
    int failed = 0;

    for (size_t i = 0; i < NUM_TESTS; i++) {
        printf("Running: %s... ", s_tests[i].name);
        fflush(stdout);

        int result = s_tests[i].func();
        if (result == 0) {
            printf("PASS\n");
            passed++;
        } else {
            printf("FAIL\n");
            failed++;
        }
    }

    printf("\n========================================\n");
    printf("Results: %d passed, %d failed, %zu total\n", passed, failed, NUM_TESTS);
    printf("========================================\n");

    return failed > 0 ? 1 : 0;
}
