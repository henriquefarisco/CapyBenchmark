# CapyBenchmark compatibility and integration contract

CapyBenchmark owns the **portable benchmark harness, report model
and baseline logic**. CapyBenchmark modules must remain portable
and not own real clocks, scheduler instrumentation or release gate
orchestration.

## CapyOS reference version

- CapyOS core pinned for this contract: `0.8.0-alpha.262+20260602`
- Authoritative cross-repo matrix: [`CapyOS/docs/reference/integration/compatibility-matrix.md`](../../CapyOS/docs/reference/integration/compatibility-matrix.md)
- Canonical manifest format consumed by the in-tree adapter: [`CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`](../../CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md)
- Manual deploy runbook: [`CapyOS/docs/operations/manual-module-deploy-runbook.md`](../../CapyOS/docs/operations/manual-module-deploy-runbook.md)
- Current cross-repo audit: [`CapyOS/docs/reference/integration/compatibility-audit-2026-06-02.md`](../../CapyOS/docs/reference/integration/compatibility-audit-2026-06-02.md)

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
- all-violations query (`capy_benchmark_failing_metrics`) returning a bitmask
  of every threshold a report breaches at once (complements the first-failure
  `capy_benchmark_evaluate` verdict);
- serialization format (v1, additive): line-oriented `key=value`
  like capypkg manifests — see "Report serialization format" and
  "Evaluation serialization format" below.

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

## Report serialization format

`capy_benchmark_report_serialize` (declared in
`src/harness/capy_benchmark.h`) emits a deterministic, line-oriented
`key=value` text form aligned with the capypkg manifest model. It is a
pure function of the report struct: the same report always produces
byte-for-byte identical output.

Rules:

- ASCII printable only (0x20-0x7E); each record is `key=value` ended
  by a single `\n`.
- Fields are emitted in a fixed order and the form is terminated by a
  `---` sentinel line, matching the capypkg manifest convention.
- Keys map 1:1 to report fields: `name`, `benchmark_version`,
  `runner_version`, `platform`, `replay_id`, `seed`, then the eight
  metrics (`average_fps_milli`, `p95_frame_time_us`,
  `p99_frame_time_us`, `input_latency_us`, `cpu_usage_milli`,
  `memory_peak_kib`, `dropped_events`, `state_checksum`).
- Numeric values are unsigned decimal (endian-neutral).
- Fail-closed: serialization returns `0` and empties the output buffer
  for an invalid report, a string field with non-printable bytes, or a
  buffer too small for the full form. No partial form is ever returned.
- Additive evolution: future keys may be appended before the `---`
  sentinel; existing keys never change meaning.
- `CAPY_BENCHMARK_SERIAL_MAX` (1024) bounds the worst-case form;
  callers should size buffers accordingly.

Example (the `snake-frame` host fixture):

```
name=snake-frame
benchmark_version=0.0.1
runner_version=0.0.1
platform=host
replay_id=7
seed=42
average_fps_milli=60000
p95_frame_time_us=17000
p99_frame_time_us=18000
input_latency_us=4000
cpu_usage_milli=250
memory_peak_kib=4096
dropped_events=0
state_checksum=12648430
---
```

## Evaluation serialization format

`capy_benchmark_evaluation_serialize` (declared in
`src/harness/capy_benchmark.h`) emits the pass/fail verdict in the same
line-oriented `key=value` form, so a report and its evaluation can be
written into one artifact. It is a pure function of the evaluation
struct.

Rules:

- `result=<code>` where `<code>` is one of the stable strings `pass`,
  `regression`, `invalid_report` or `unsupported`, mapped from
  `enum capy_benchmark_result_code`.
- `reason=<reason>` carries the printable pass/failure reason.
- The form is terminated by the `---` sentinel line.
- Fail-closed: serialization returns `0` and empties the output buffer
  for an unknown result code, an empty or non-printable reason, or a
  buffer too small for the full form.
- The worst-case form fits within `CAPY_BENCHMARK_SERIAL_MAX`.

Example (a regression verdict):

```
result=regression
reason=average fps below baseline
---
```

## Replay metadata format

`capy_benchmark_replay_serialize` emits the deterministic replay inputs
in the same line-oriented `key=value` form. A replay is identified by
`replay_id` + `seed` (the same pair carried in the report) plus a
`frame_budget` (number of frames the deterministic run executes).

Rules:

- Keys are `replay_id`, `seed`, `frame_budget`, terminated by the `---`
  sentinel line.
- `frame_budget` must be non-zero; `capy_benchmark_replay_valid` rejects
  a zero budget and serialization fails closed (`0`, output emptied).
- Same replay metadata → byte-for-byte identical output and, by the
  determinism contract, the same `metrics.state_checksum`.
- The per-frame input trajectory is intentionally not encoded yet: its
  shape depends on the `capy-lang-host` input event model (Etapa 15)
  and will be added additively (new keys before `---`) once it
  stabilizes.

Example:

```
replay_id=7
seed=42
frame_budget=600
---
```

The read side is `capy_benchmark_replay_parse` — the canonical inverse of
`capy_benchmark_replay_serialize`. It parses a bounded `(text, len)` buffer
back into a `capy_benchmark_replay` and round-trips byte-for-byte: serialising
the parsed struct reproduces the input bytes. It is a pure function of the
input bytes (no clocks, allocation or globals).

Rules:

- Accepts only the canonical form: the keys `replay_id`, `seed`,
  `frame_budget` in that order, each `key=value` ended by a single `\n`,
  then the `---` sentinel line, and the buffer must be consumed exactly
  (`len` bounds the scan; the parser never reads past it).
- Values are unsigned decimal with no leading zero (except a lone `0`) and
  must fit in `uint32_t`.
- Fail-closed: returns `0` and zeroes the output struct for a missing,
  reordered or unknown key, a non-decimal / overflowing / empty /
  non-canonical value, a missing `---` sentinel, trailing bytes, a zero
  `frame_budget`, or a NULL buffer. No partial struct is ever returned.
- Additive: this is a new read-side entry point; the serialized format and
  its bytes are unchanged.

The report read side is `capy_benchmark_report_parse` — the canonical inverse
of `capy_benchmark_report_serialize`. It parses the 4 string fields (printable
ASCII, non-empty, each capped to its field size), `replay_id`, `seed` and the
8 metric fields, requires the exact key order plus the `---` sentinel, consumes
the buffer exactly, and round-trips byte-for-byte. It fails closed (zeroing
`out`) on a reordered/unknown/missing key, an empty or non-printable string
value, an over-long value, a non-canonical/overflowing number, trailing bytes
or a NULL buffer. Additive: the serialized bytes are unchanged.

The evaluation read side is `capy_benchmark_evaluation_parse` — the canonical
inverse of `capy_benchmark_evaluation_serialize`, completing the read-side
trilogy (replay, report, evaluation). It parses the `result` token back to its
`capy_benchmark_result_code`, the printable-ASCII non-empty `reason`, requires
the `---` sentinel, consumes the buffer exactly, and round-trips byte-for-byte.
It fails closed (zeroing `out`) on a reordered/unknown/missing key, an empty or
non-printable `reason`, an unrecognised `result` token (one the serialiser would
never emit), trailing bytes or a NULL buffer. Additive: the serialized bytes are
unchanged.

## Baseline-derived thresholds

`capy_benchmark_thresholds_from_baseline` (declared in
`src/harness/capy_benchmark.h`) derives a `capy_benchmark_thresholds` set from
a known-good baseline report plus a tolerance in parts-per-1000 (`0..1000` =
0%..100%): the fps metric gets a floor `tolerance` below the baseline, every
other metric a ceiling `tolerance` above it, and the deterministic state
checksum is pinned exactly (`require_state_checksum = 1`). Feeding the result
to `capy_benchmark_evaluate` turns a recorded baseline run into a regression
gate for later runs — the Etapa-16 regressive-baseline workflow. It is a pure
function and fails closed (returns 0, zeroing `out`) for a NULL/invalid
baseline or a tolerance above 1000. `report_valid` guarantees the baseline's
fps/p95/p99 are non-zero, so those bounds are always meaningful; a baseline
metric of 0 (e.g. `dropped_events`) yields a 0 threshold, which `evaluate`
treats as unchecked.

`capy_benchmark_thresholds_serialize` / `capy_benchmark_thresholds_parse`
round-trip a threshold set through the same line-oriented `key=value` form (the
9 fields in declaration order + the `---` sentinel), completing the harness's
serialization surface (report, evaluation, replay, thresholds) so a derived or
hand-tuned regression gate can be persisted and version-controlled directly.
The parser is the canonical inverse — byte-for-byte round-trip, fail-closed on
a reordered/unknown/missing key, a non-canonical number, trailing bytes or a
NULL buffer; every threshold combination is semantically valid, so there is no
extra gate beyond the structural one.

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
