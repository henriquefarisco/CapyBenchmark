# CapyBenchmark compatibility and integration contract

CapyBenchmark owns the **portable benchmark harness, report model
and baseline logic**. CapyBenchmark modules must remain portable
and not own real clocks, scheduler instrumentation or release gate
orchestration.

## CapyOS reference version

- CapyOS core pinned for this contract: `0.8.0-alpha.244+20260520`
- Authoritative cross-repo matrix: [`CapyOS/docs/reference/integration/compatibility-matrix.md`](../../CapyOS/docs/reference/integration/compatibility-matrix.md)
- Canonical manifest format consumed by the in-tree adapter: [`CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`](../../CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md)
- Manual deploy runbook: [`CapyOS/docs/operations/manual-module-deploy-runbook.md`](../../CapyOS/docs/operations/manual-module-deploy-runbook.md)
- Current cross-repo audit: [`CapyOS/docs/reference/integration/compatibility-audit-2026-05-20.md`](../../CapyOS/docs/reference/integration/compatibility-audit-2026-05-20.md)

Authoritative CapyOS references:

- `CapyOS/docs/reference/integration/modular-installation-architecture.md`
- `CapyOS/docs/reference/integration/benchmark-harness-integration-contract.md`
- `CapyOS/docs/reference/integration/external-core-repositories.md`

## Owned ABI

CapyBenchmark owns the `capy-benchmark-report` ABI (v1, planned —
runtime integration gated by Etapas 15-16).

This ABI covers:

- benchmark report model (workload, channel, baseline, metrics,
  pass/fail);
- baseline threshold model (per-workload tolerances);
- replay metadata when available (seed, frame budget, input
  trajectory);
- deterministic pass/fail evaluation (same inputs → same verdict);
- serialization format once introduced (planned: line-oriented
  `key=value` like capypkg manifests; final shape TBD).

CapyBenchmark does **not** own:

- real clock source (CapyOS owns; benchmark consumes monotonic
  `uint32_t` ms ticks);
- scheduler / compositor metrics collection (CapyOS owns);
- platform identity (CapyOS or host adapter supplies);
- release gate orchestration (CapyOS `make release-check`);
- staging / activation / rollback (CapyOS adapter);
- direct CapyOS kernel includes;
- direct workload execution (CapyLang VM executes when workload uses
  CapyLang bytecode).

## Compatibility rules

- Report fields must be additive until the integration stage
  permits migration.
- Baseline comparisons must be deterministic for the same inputs.
- Platform identity must be supplied by CapyOS or a host adapter,
  not inferred from kernel internals.
- Benchmarks must declare workload, channel and baseline
  compatibility in the report.
- CapyBenchmark must not own real clocks, scheduler instrumentation
  or release gate orchestration.
- Same workload + same inputs + same baseline → same pass/fail
  verdict, byte-for-byte serialised report.

## Error model

| Code family | Trigger | Caller behaviour |
|---|---|---|
| Report field invalid | parser rejects malformed report | report discarded; release gate fails closed |
| Baseline threshold absent | report references unknown baseline | report flagged "no baseline"; not a failure |
| Workload deterministic mismatch | rerun with same inputs produces different metrics | benchmark fails; not a release blocker if marked non-deterministic |
| Platform identity unspecified | report lacks platform tuple (arch + VM + driver set) | report cannot be compared to baseline; flagged |
| Pass/fail evaluation overflow | metric exceeds representable range | report flagged invalid |

All errors must be deterministic. CapyBenchmark never produces
indeterminate verdicts and never reads platform internals directly.

## Resource and performance limits

| Limit | Value | Owner |
|---|---|---|
| Report serialized size | bounded by Capy package payload limit | CapyOS adapter |
| Replay seed determinism | fixed 64-bit seed per workload | CapyBenchmark |
| Frame budget per benchmark run | configurable; bounded by host adapter | CapyOS host ABI |
| Maximum workload duration | configurable; release gate enforces | CapyOS host adapter |
| Capy package payload | ≤ 1 MiB during the alpha streaming-buffer window | CapyOS adapter |

## Install/update boundary

CapyBenchmark may be installed as an optional validation / reporting
component when Etapas 15-16 open. CapyOS owns:

- real clock source;
- scheduler / compositor metrics;
- platform identity;
- release gate orchestration;
- staging, activation and rollback.

## Dependency rules

Benchmark components may depend on:

- `capy-benchmark-report` (always when emitting reports);
- `capy-lang-host` for CapyLang workloads (e.g. Snake/Asteroids in
  Etapa 15);
- explicit CapyOS measurement adapter ABIs when they exist.

They must not depend on runtime internals directly.

## Validation before CapyOS integration

Before CapyOS consumes a CapyBenchmark release, externally validate:

- report model fixtures (golden);
- baseline pass/fail fixtures;
- malformed report rejection;
- deterministic replay behaviour when available;
- CapyLang workload compatibility (when included);
- `make validate` and `make package` produce canonical assets when
  the integration stage opens.

CapyBenchmark integration is gated by Etapas 15-16.

## Publishing as a Capy package (Etapas 15-16, when the stage opens)

When CapyBenchmark is delivered as a remote module to the CapyOS
`services/capypkg` adapter, the publisher must follow
[`CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`](../../CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md).
The key requirements that affect CapyBenchmark are:

- `payload_url` must be HTTPS only;
- `payload_sha256` must be lowercase 64 hex of the published artifact;
- `payload_size` ≤ 1 MiB during the alpha streaming-buffer window;
- `name` must follow `[a-zA-Z0-9._-]`; suggested canonical name
  `org.capyos.benchmark.harness`;
- `install_root` must live under `/var/capypkg` or `/opt/`;
- `signature_ed25519` must cover the canonical descriptor
  `name=N|version=V|payload_sha256=H|payload_url=U\n`;
- `depends` must include the CapyLang runtime package name when the
  benchmark requires the VM (`org.capyos.lang.runtime` or the agreed
  CapyLang canonical name).

Until CapyAgent publishes its Ed25519 signer, CapyBenchmark cannot
be installed from a `signed` repository in production.
