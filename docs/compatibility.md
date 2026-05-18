# CapyBenchmark compatibility and integration contract

CapyBenchmark modules must remain portable benchmark harness and baseline logic.

Authoritative CapyOS references:

- `CapyOS/docs/reference/integration/modular-installation-architecture.md`
- `CapyOS/docs/reference/integration/benchmark-harness-integration-contract.md`

## Owned ABI

CapyBenchmark owns the `capy-benchmark-report` ABI.

This ABI covers:

- benchmark report model;
- baseline threshold model;
- replay metadata when available;
- deterministic pass/fail evaluation;
- serialization format once introduced.

## Compatibility rules

- Report fields must be additive until the integration stage permits migration.
- Baseline comparisons must be deterministic for the same inputs.
- Platform identity must be supplied by CapyOS or a host adapter, not inferred from kernel internals.
- Benchmarks must declare workload, channel and baseline compatibility.
- CapyBenchmark must not own real clocks, scheduler instrumentation or release gate orchestration.

## Install/update boundary

CapyBenchmark may be installed as an optional validation/reporting component. CapyOS owns:

- real clock source;
- scheduler/compositor metrics;
- platform identity;
- release gate orchestration;
- staging, activation and rollback.

## Dependency rules

Benchmark components may depend on:

- `capy-benchmark-report`;
- `capy-lang-host` for CapyLang workloads;
- explicit CapyOS measurement adapter ABIs when they exist.

They must not depend on runtime internals directly.

## Validation before CapyOS integration

Before CapyOS consumes a CapyBenchmark release, externally validate:

- report model fixtures;
- baseline pass/fail fixtures;
- malformed report rejection;
- deterministic replay behavior when available;
- CapyLang workload compatibility when included.

CapyBenchmark integration is gated by Etapas 15-16.
