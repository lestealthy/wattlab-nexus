# Release Process

How WattLab Nexus versions are cut. Pre-1.0 releases use `0.x.y` with
pre-release labels; a build is only "stable" after hardware validation.

## Version scheme

```
MAJOR.MINOR.PATCH[-label]
```

Examples:
- `0.1.0` — foundation (software-verified)
- `0.2.0-beta.1` — hardware-integrated, pending physical validation
- `1.0.0` — after the hardware test plan passes and the API stabilises

The canonical version lives in `firmware/include/nexus_version.h`
(`NEXUS_VERSION_STRING`).

## Pre-release labels

- `-alpha` — architecture/prototype; not hardware-verified.
- `-beta.N` — feature-complete for the milestone; awaiting hardware validation.
- no label — hardware-validated and stable.

## Cutting a release

1. **Update the version** in `firmware/include/nexus_version.h`.
2. **Update `CHANGELOG.md`**: move entries from `[Unreleased]` into a new
   version section with the date.
3. **Run the full verification**:
   ```
   scripts\test-all.bat
   ```
   It must report `TOTAL: PASS` (host tests, simulator, firmware build, lint).
4. **Record build metrics**: capture flash/RAM from the build output and note
   them in the release note.
5. **Write/refresh the release note** under `docs/` (see
   `RELEASE_NOTE_0.2.0-beta.1.md` as a template).
6. **Do not claim hardware success** unless `docs/ALPHA_HARDWARE_RESULTS.md`
   has been updated from physical testing.
7. **Tag and push**:
   ```
   git add -A
   git commit -m "release: vX.Y.Z-label"
   git tag -a vX.Y.Z-label -m "WattLab Nexus vX.Y.Z-label"
   git push origin main --tags
   ```
8. **Create a GitHub release** for the tag, pre-release marked for beta/alpha,
   with the release note as the body.

## Hardware validation gate

Before removing a `-beta` label:

- Complete every test in `docs/ALPHA_TEST_PLAN.md` on a real board.
- Fill in `docs/ALPHA_HARDWARE_RESULTS.md` with recorded serial output.
- Confirm the SD and RTC rows are `PASS`, and document the RTC
  persistence behaviour actually observed.

## Repository settings

- The repository is **private** during development.
- Releases with pre-release labels are marked as "pre-release" on GitHub.
