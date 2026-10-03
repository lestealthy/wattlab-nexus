#ifndef NEXUS_STORAGE_HOST_H
#define NEXUS_STORAGE_HOST_H

#include "nexus_storage.h"

/*
 * Host (desktop) storage backend used by the unit tests and simulator.
 *
 * Maps logical Nexus paths (e.g. "/NEXUS/LOGS/x.log") onto a real directory
 * tree supplied by the caller. This lets the entire storage service - path
 * safety, layout creation, atomic writes, temp recovery - run unmodified on
 * the host, giving real filesystem behaviour in tests.
 */

// Configure the backend root. Pass an existing, writable directory.
void nexus_storage_host_set_root(const char* root_dir);

// Returns the backend vtable for use with nexus_storage_select().
nexus_storage_backend_t* nexus_storage_host_backend(void);

// Test helpers to exercise fault paths without touching real world state.
void nexus_storage_host_inject_full(bool full);
void nexus_storage_host_inject_write_failure(bool fail);
void nexus_storage_host_inject_mount_failure(bool fail);

#endif
