# CapyBenchmark documentation

CapyBenchmark owns portable benchmark harness logic for CapyOS and CapyLang workloads.

## CapyOS reference version

Pinned for this release: `0.8.0-alpha.262+20260602`. Update this together with `docs/compatibility.md` whenever the CapyOS core version, ABI or canonical manifest format changes.

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

1. Report serialization (line-oriented `key=value`) — **done** (`capy_benchmark_report_serialize`; see `compatibility.md`).
2. Evaluation serialization (`result` + `reason`) — **done** (`capy_benchmark_evaluation_serialize`; see `compatibility.md`).
3. Baseline fixture tests (golden serialized reports per workload + strict/VM baselines) — **done** (`tests/test_benchmark_contracts.c`).
4. Replay metadata (`replay_id`, `seed`, `frame_budget`) — **done** (`capy_benchmark_replay`); per-frame input trajectory deferred until the `capy-lang-host` input model stabilizes (Etapa 15).
5. Add CapyLang Snake/Asteroids benchmark contracts (via `capy-lang-host`).
6. Add CapyOS adapter only when Etapas 15-16 permit integration.
