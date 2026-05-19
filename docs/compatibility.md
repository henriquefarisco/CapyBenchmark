# CapyBenchmark compatibility and integration contract

CapyBenchmark modules must remain portable benchmark harness and baseline logic.

## CapyOS reference version

- CapyOS core pinned for this contract: `0.8.0-alpha.240+20260519`
- Authoritative cross-repo matrix: `CapyOS/docs/reference/integration/compatibility-matrix.md`
- Canonical manifest format consumed by the in-tree adapter: `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`
- Manual deploy runbook: `CapyOS/docs/operations/manual-module-deploy-runbook.md`

Authoritative CapyOS references:

- `CapyOS/docs/reference/integration/modular-installation-architecture.md`
- `CapyOS/docs/reference/integration/benchmark-harness-integration-contract.md`
- `CapyOS/docs/reference/integration/compatibility-matrix.md`
- `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`

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

## Publishing as a Capy package (Etapas 15-16, when the stage opens)

When CapyBenchmark is delivered as a remote module to the CapyOS
`services/capypkg` adapter, the publisher must follow
`CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`.
The key requirements that affect CapyBenchmark are:

- `payload_url` must be HTTPS only;
- `payload_sha256` must be lowercase 64 hex of the published artifact;
- `payload_size` ≤ 1 MiB during the alpha streaming-buffer window;
- `name` must follow `[a-zA-Z0-9._-]` (suggested `org.capyos.benchmark.harness`);
- `install_root` must live under `/var/capypkg` or `/opt/`;
- `signature_ed25519` must cover the canonical descriptor
  `name=N|version=V|payload_sha256=H|payload_url=U\n`;
- `depends` must include the CapyLang runtime package name when the
  benchmark requires the VM (`org.capyos.lang.runtime` or the agreed
  CapyLang name).

Until CapyAgent publishes its Ed25519 signer, CapyBenchmark cannot
be installed from a `signed` repository in production.
