#ifndef NEXUS_BOOT_H
#define NEXUS_BOOT_H

#include "nexus_errors.h"

/*
 * Boot-time helpers that run in normal context after storage is available.
 * In particular, a crash recorded during a previous run (kept in backup SRAM
 * on device) is written to /NEXUS/CRASHES/ here - never inside the fault
 * handler, which must not touch the filesystem.
 */

// If a crash record exists AND storage is ready, persist it to
// /NEXUS/CRASHES/CRASH_<stamp>.json and return NEXUS_OK. Returns
// NEXUS_ERR_NOT_FOUND if there is no record to persist.
nexus_err_t nexus_boot_persist_crash(void);

#endif
