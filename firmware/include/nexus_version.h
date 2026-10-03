#ifndef NEXUS_VERSION_H
#define NEXUS_VERSION_H

/*
 * WattLab Nexus version.
 *
 * Release naming during pre-1.0 development:
 *   0.1.0             software-verified foundation (Alpha, no hardware)
 *   0.2.0-beta.1      hardware-integrated, pending physical validation (Beta)
 *
 * The user-facing label for 0.2.0-beta.x is "Beta" until the board has been
 * validated on real hardware (see docs/ALPHA_TEST_PLAN.md and
 * docs/ALPHA_HARDWARE_RESULTS.md), after which it can be promoted to a stable
 * release.
 */

#define NEXUS_VERSION_MAJOR 0
#define NEXUS_VERSION_MINOR 2
#define NEXUS_VERSION_PATCH 0
#define NEXUS_VERSION_PRERELEASE "beta.1"

#define NEXUS_VERSION_STRING "0.2.0-beta.1"
#define NEXUS_VERSION_FULL   "0.2.0-beta.1"

#define NEXUS_BUILD_DATE __DATE__
#define NEXUS_BUILD_TIME __TIME__

#endif
