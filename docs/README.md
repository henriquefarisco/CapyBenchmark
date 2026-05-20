# CapyBenchmark documentation

CapyBenchmark owns portable benchmark harness logic for CapyOS and CapyLang workloads.

## CapyOS reference version

Pinned for this release: `0.8.0-alpha.241+20260519`. Update this together with `docs/compatibility.md` whenever the CapyOS core version, ABI or canonical manifest format changes.

Cross-repo authoritative references:

- `CapyOS/docs/reference/integration/compatibility-matrix.md`
- `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`
- `CapyOS/docs/operations/manual-module-deploy-runbook.md`

## Current migration

No coupled CapyOS benchmark harness implementation was found. This repository starts with a small host-testable core:

- `src/harness/capy_benchmark.h`
- `src/harness/capy_benchmark.c`
- `docs/compatibility.md`

## Boundary

CapyBenchmark owns:

- benchmark report model;
- baseline thresholds;
- deterministic pass/fail evaluation;
- future replay format;
- future serialization format;
- game benchmark contracts for CapyLang workloads.

CapyOS owns:

- real clocks;
- scheduler metrics;
- compositor/backend instrumentation;
- VM platform identity;
- external release gate orchestration.

## Tag-release compatibility model

Early alpha releases use GitHub release tags plus a compatibility index without certificate/signature enforcement. Required metadata:

- component id;
- tag;
- artifact name;
- sha256;
- required CapyOS/CapyLang ABI versions;
- benchmark channel;
- baseline compatibility.

## Next slices

1. Add replay input model.
2. Add report serialization.
3. Add baseline fixture tests.
4. Add CapyLang Snake/Asteroids benchmark contracts.
5. Add CapyOS adapter only when Etapas 15-16 permit integration.
