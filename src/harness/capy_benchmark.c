#include "capy_benchmark.h"

static void capy_benchmark_zero(void *ptr, size_t len) {
  uint8_t *p = (uint8_t *)ptr;
  while (len--) {
    *p++ = 0;
  }
}

static void capy_benchmark_copy(char *dst, size_t dst_size, const char *src) {
  size_t i = 0u;
  if (!dst || dst_size == 0u) {
    return;
  }
  if (!src) {
    dst[0] = '\0';
    return;
  }
  while (i + 1u < dst_size && src[i]) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = '\0';
}

static void capy_benchmark_set_result(struct capy_benchmark_evaluation *out,
                                      enum capy_benchmark_result_code code,
                                      const char *reason) {
  if (!out) {
    return;
  }
  out->code = code;
  capy_benchmark_copy(out->reason, sizeof(out->reason), reason);
}

void capy_benchmark_report_init(struct capy_benchmark_report *report,
                                const char *name,
                                const char *benchmark_version,
                                const char *runner_version,
                                const char *platform) {
  if (!report) {
    return;
  }
  capy_benchmark_zero(report, sizeof(*report));
  capy_benchmark_copy(report->name, sizeof(report->name), name);
  capy_benchmark_copy(report->benchmark_version,
                      sizeof(report->benchmark_version), benchmark_version);
  capy_benchmark_copy(report->runner_version, sizeof(report->runner_version),
                      runner_version);
  capy_benchmark_copy(report->platform, sizeof(report->platform), platform);
}

int capy_benchmark_report_valid(const struct capy_benchmark_report *report) {
  if (!report || !report->name[0] || !report->benchmark_version[0] ||
      !report->runner_version[0] || !report->platform[0]) {
    return 0;
  }
  if (report->metrics.average_fps_milli == 0u ||
      report->metrics.p95_frame_time_us == 0u ||
      report->metrics.p99_frame_time_us == 0u) {
    return 0;
  }
  return 1;
}

void capy_benchmark_evaluate(const struct capy_benchmark_report *report,
                             const struct capy_benchmark_thresholds *thresholds,
                             struct capy_benchmark_evaluation *out) {
  if (!out) {
    return;
  }
  capy_benchmark_zero(out, sizeof(*out));
  if (!report || !thresholds || !capy_benchmark_report_valid(report)) {
    capy_benchmark_set_result(out, CAPY_BENCHMARK_FAIL_INVALID_REPORT,
                              "invalid report");
    return;
  }
  if (thresholds->min_average_fps_milli &&
      report->metrics.average_fps_milli < thresholds->min_average_fps_milli) {
    capy_benchmark_set_result(out, CAPY_BENCHMARK_FAIL_REGRESSION,
                              "average fps below baseline");
    return;
  }
  if (thresholds->max_p95_frame_time_us &&
      report->metrics.p95_frame_time_us > thresholds->max_p95_frame_time_us) {
    capy_benchmark_set_result(out, CAPY_BENCHMARK_FAIL_REGRESSION,
                              "p95 frame time above baseline");
    return;
  }
  if (thresholds->max_p99_frame_time_us &&
      report->metrics.p99_frame_time_us > thresholds->max_p99_frame_time_us) {
    capy_benchmark_set_result(out, CAPY_BENCHMARK_FAIL_REGRESSION,
                              "p99 frame time above baseline");
    return;
  }
  if (thresholds->max_input_latency_us &&
      report->metrics.input_latency_us > thresholds->max_input_latency_us) {
    capy_benchmark_set_result(out, CAPY_BENCHMARK_FAIL_REGRESSION,
                              "input latency above baseline");
    return;
  }
  if (thresholds->max_cpu_usage_milli &&
      report->metrics.cpu_usage_milli > thresholds->max_cpu_usage_milli) {
    capy_benchmark_set_result(out, CAPY_BENCHMARK_FAIL_REGRESSION,
                              "cpu usage above baseline");
    return;
  }
  if (thresholds->max_memory_peak_kib &&
      report->metrics.memory_peak_kib > thresholds->max_memory_peak_kib) {
    capy_benchmark_set_result(out, CAPY_BENCHMARK_FAIL_REGRESSION,
                              "memory peak above baseline");
    return;
  }
  if (thresholds->max_dropped_events &&
      report->metrics.dropped_events > thresholds->max_dropped_events) {
    capy_benchmark_set_result(out, CAPY_BENCHMARK_FAIL_REGRESSION,
                              "dropped events above baseline");
    return;
  }
  if (thresholds->require_state_checksum &&
      report->metrics.state_checksum != thresholds->expected_state_checksum) {
    capy_benchmark_set_result(out, CAPY_BENCHMARK_FAIL_REGRESSION,
                              "state checksum mismatch");
    return;
  }
  capy_benchmark_set_result(out, CAPY_BENCHMARK_PASS, "pass");
}

uint32_t capy_benchmark_failing_metrics(
    const struct capy_benchmark_report *report,
    const struct capy_benchmark_thresholds *thresholds) {
  uint32_t flags = 0u;
  if (!report || !thresholds || !capy_benchmark_report_valid(report)) {
    return CAPY_BENCHMARK_METRIC_INVALID_REPORT;
  }
  if (thresholds->min_average_fps_milli &&
      report->metrics.average_fps_milli < thresholds->min_average_fps_milli) {
    flags |= CAPY_BENCHMARK_METRIC_AVERAGE_FPS;
  }
  if (thresholds->max_p95_frame_time_us &&
      report->metrics.p95_frame_time_us > thresholds->max_p95_frame_time_us) {
    flags |= CAPY_BENCHMARK_METRIC_P95_FRAME_TIME;
  }
  if (thresholds->max_p99_frame_time_us &&
      report->metrics.p99_frame_time_us > thresholds->max_p99_frame_time_us) {
    flags |= CAPY_BENCHMARK_METRIC_P99_FRAME_TIME;
  }
  if (thresholds->max_input_latency_us &&
      report->metrics.input_latency_us > thresholds->max_input_latency_us) {
    flags |= CAPY_BENCHMARK_METRIC_INPUT_LATENCY;
  }
  if (thresholds->max_cpu_usage_milli &&
      report->metrics.cpu_usage_milli > thresholds->max_cpu_usage_milli) {
    flags |= CAPY_BENCHMARK_METRIC_CPU_USAGE;
  }
  if (thresholds->max_memory_peak_kib &&
      report->metrics.memory_peak_kib > thresholds->max_memory_peak_kib) {
    flags |= CAPY_BENCHMARK_METRIC_MEMORY_PEAK;
  }
  if (thresholds->max_dropped_events &&
      report->metrics.dropped_events > thresholds->max_dropped_events) {
    flags |= CAPY_BENCHMARK_METRIC_DROPPED_EVENTS;
  }
  if (thresholds->require_state_checksum &&
      report->metrics.state_checksum != thresholds->expected_state_checksum) {
    flags |= CAPY_BENCHMARK_METRIC_STATE_CHECKSUM;
  }
  return flags;
}

static uint32_t capy_benchmark_clamp_u32(uint64_t value) {
  return value > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)value;
}

/* Ceiling = base scaled up by tolerance (parts-per-1000), clamped to u32.
 * A base of 0 yields 0, which evaluate() treats as "no bound" for that
 * metric. */
static uint32_t capy_benchmark_scaled_ceiling(uint32_t base,
                                              uint32_t tolerance_milli) {
  return capy_benchmark_clamp_u32((uint64_t)base * (1000u + tolerance_milli) /
                                  1000u);
}

int capy_benchmark_thresholds_from_baseline(
    const struct capy_benchmark_report *baseline, uint32_t tolerance_milli,
    struct capy_benchmark_thresholds *out) {
  if (!out) {
    return 0;
  }
  capy_benchmark_zero(out, sizeof(*out));
  /* report_valid also rejects a NULL baseline and guarantees fps/p95/p99 are
   * non-zero, so the derived fps floor and frame-time ceilings are meaningful
   * bounds rather than evaluate()'s "0 = unchecked" sentinel. */
  if (!capy_benchmark_report_valid(baseline)) {
    return 0;
  }
  if (tolerance_milli > 1000u) {
    return 0; /* tolerance is 0..1000 parts-per-1000 (0%..100%) */
  }
  /* fps is a floor: allow it to drop by up to tolerance below the baseline. */
  out->min_average_fps_milli =
      (uint32_t)((uint64_t)baseline->metrics.average_fps_milli *
                 (1000u - tolerance_milli) / 1000u);
  /* the remaining metrics are ceilings: allow them to rise by up to
   * tolerance above the baseline. */
  out->max_p95_frame_time_us =
      capy_benchmark_scaled_ceiling(baseline->metrics.p95_frame_time_us,
                                    tolerance_milli);
  out->max_p99_frame_time_us =
      capy_benchmark_scaled_ceiling(baseline->metrics.p99_frame_time_us,
                                    tolerance_milli);
  out->max_input_latency_us =
      capy_benchmark_scaled_ceiling(baseline->metrics.input_latency_us,
                                    tolerance_milli);
  out->max_cpu_usage_milli =
      capy_benchmark_scaled_ceiling(baseline->metrics.cpu_usage_milli,
                                    tolerance_milli);
  out->max_memory_peak_kib =
      capy_benchmark_scaled_ceiling(baseline->metrics.memory_peak_kib,
                                    tolerance_milli);
  out->max_dropped_events =
      capy_benchmark_scaled_ceiling(baseline->metrics.dropped_events,
                                    tolerance_milli);
  /* a baseline pins the deterministic state checksum exactly. */
  out->expected_state_checksum = baseline->metrics.state_checksum;
  out->require_state_checksum = 1u;
  return 1;
}

static int capy_benchmark_append(char *dst, size_t cap, size_t *pos,
                                 const char *src) {
  size_t i = 0u;
  size_t p;
  size_t avail;
  size_t k = 0u;
  if (!dst || !pos || cap == 0u || !src) {
    return 0;
  }
  p = *pos;
  while (src[i]) {
    ++i;
  }
  if (p > cap - 1u) {
    return 0;
  }
  avail = (cap - 1u) - p;
  if (i > avail) {
    return 0;
  }
  while (k < i) {
    dst[p + k] = src[k];
    ++k;
  }
  *pos = p + i;
  return 1;
}

static int capy_benchmark_append_u32(char *dst, size_t cap, size_t *pos,
                                     uint32_t value) {
  char tmp[11];
  char rev[10];
  size_t n = 0u;
  size_t r = 0u;
  if (value == 0u) {
    rev[r++] = '0';
  } else {
    while (value > 0u) {
      rev[r++] = (char)('0' + (value % 10u));
      value /= 10u;
    }
  }
  while (r > 0u) {
    tmp[n++] = rev[--r];
  }
  tmp[n] = '\0';
  return capy_benchmark_append(dst, cap, pos, tmp);
}

static int capy_benchmark_str_serializable(const char *s) {
  size_t i = 0u;
  if (!s || !s[0]) {
    return 0;
  }
  while (s[i]) {
    int c = (int)(unsigned char)s[i];
    if (c < 0x20 || c > 0x7E) {
      return 0;
    }
    ++i;
  }
  return 1;
}

static int capy_benchmark_append_kv_str(char *dst, size_t cap, size_t *pos,
                                        const char *key, const char *val) {
  return capy_benchmark_append(dst, cap, pos, key) &&
         capy_benchmark_append(dst, cap, pos, "=") &&
         capy_benchmark_append(dst, cap, pos, val) &&
         capy_benchmark_append(dst, cap, pos, "\n");
}

static int capy_benchmark_append_kv_u32(char *dst, size_t cap, size_t *pos,
                                        const char *key, uint32_t val) {
  return capy_benchmark_append(dst, cap, pos, key) &&
         capy_benchmark_append(dst, cap, pos, "=") &&
         capy_benchmark_append_u32(dst, cap, pos, val) &&
         capy_benchmark_append(dst, cap, pos, "\n");
}

int capy_benchmark_report_serialize(const struct capy_benchmark_report *report,
                                    char *out, size_t out_size) {
  size_t pos = 0u;
  int ok;
  if (!out || out_size == 0u) {
    return 0;
  }
  out[0] = '\0';
  if (!report || !capy_benchmark_report_valid(report)) {
    return 0;
  }
  if (!capy_benchmark_str_serializable(report->name) ||
      !capy_benchmark_str_serializable(report->benchmark_version) ||
      !capy_benchmark_str_serializable(report->runner_version) ||
      !capy_benchmark_str_serializable(report->platform)) {
    return 0;
  }
  ok = capy_benchmark_append_kv_str(out, out_size, &pos, "name",
                                    report->name) &&
       capy_benchmark_append_kv_str(out, out_size, &pos, "benchmark_version",
                                    report->benchmark_version) &&
       capy_benchmark_append_kv_str(out, out_size, &pos, "runner_version",
                                    report->runner_version) &&
       capy_benchmark_append_kv_str(out, out_size, &pos, "platform",
                                    report->platform) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "replay_id",
                                    report->replay_id) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "seed",
                                    report->seed) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "average_fps_milli",
                                    report->metrics.average_fps_milli) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "p95_frame_time_us",
                                    report->metrics.p95_frame_time_us) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "p99_frame_time_us",
                                    report->metrics.p99_frame_time_us) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "input_latency_us",
                                    report->metrics.input_latency_us) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "cpu_usage_milli",
                                    report->metrics.cpu_usage_milli) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "memory_peak_kib",
                                    report->metrics.memory_peak_kib) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "dropped_events",
                                    report->metrics.dropped_events) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "state_checksum",
                                    report->metrics.state_checksum) &&
       capy_benchmark_append(out, out_size, &pos, "---\n");
  if (!ok) {
    out[0] = '\0';
    return 0;
  }
  out[pos] = '\0';
  return (int)pos;
}

static const char *capy_benchmark_result_name(
    enum capy_benchmark_result_code code) {
  switch (code) {
    case CAPY_BENCHMARK_PASS:
      return "pass";
    case CAPY_BENCHMARK_FAIL_REGRESSION:
      return "regression";
    case CAPY_BENCHMARK_FAIL_INVALID_REPORT:
      return "invalid_report";
    case CAPY_BENCHMARK_FAIL_UNSUPPORTED:
      return "unsupported";
  }
  return 0;
}

int capy_benchmark_evaluation_serialize(
    const struct capy_benchmark_evaluation *eval, char *out, size_t out_size) {
  size_t pos = 0u;
  const char *result;
  int ok;
  if (!out || out_size == 0u) {
    return 0;
  }
  out[0] = '\0';
  if (!eval) {
    return 0;
  }
  result = capy_benchmark_result_name(eval->code);
  if (!result || !capy_benchmark_str_serializable(eval->reason)) {
    return 0;
  }
  ok = capy_benchmark_append_kv_str(out, out_size, &pos, "result", result) &&
       capy_benchmark_append_kv_str(out, out_size, &pos, "reason",
                                    eval->reason) &&
       capy_benchmark_append(out, out_size, &pos, "---\n");
  if (!ok) {
    out[0] = '\0';
    return 0;
  }
  out[pos] = '\0';
  return (int)pos;
}

int capy_benchmark_replay_valid(const struct capy_benchmark_replay *replay) {
  if (!replay || replay->frame_budget == 0u) {
    return 0;
  }
  return 1;
}

int capy_benchmark_replay_serialize(const struct capy_benchmark_replay *replay,
                                    char *out, size_t out_size) {
  size_t pos = 0u;
  int ok;
  if (!out || out_size == 0u) {
    return 0;
  }
  out[0] = '\0';
  if (!capy_benchmark_replay_valid(replay)) {
    return 0;
  }
  ok = capy_benchmark_append_kv_u32(out, out_size, &pos, "replay_id",
                                    replay->replay_id) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "seed",
                                    replay->seed) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "frame_budget",
                                    replay->frame_budget) &&
       capy_benchmark_append(out, out_size, &pos, "---\n");
  if (!ok) {
    out[0] = '\0';
    return 0;
  }
  out[pos] = '\0';
  return (int)pos;
}

int capy_benchmark_thresholds_serialize(
    const struct capy_benchmark_thresholds *thresholds, char *out,
    size_t out_size) {
  size_t pos = 0u;
  int ok;
  if (!out || out_size == 0u) {
    return 0;
  }
  out[0] = '\0';
  if (!thresholds) {
    return 0;
  }
  ok = capy_benchmark_append_kv_u32(out, out_size, &pos,
                                    "min_average_fps_milli",
                                    thresholds->min_average_fps_milli) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos,
                                    "max_p95_frame_time_us",
                                    thresholds->max_p95_frame_time_us) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos,
                                    "max_p99_frame_time_us",
                                    thresholds->max_p99_frame_time_us) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos,
                                    "max_input_latency_us",
                                    thresholds->max_input_latency_us) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "max_cpu_usage_milli",
                                    thresholds->max_cpu_usage_milli) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "max_memory_peak_kib",
                                    thresholds->max_memory_peak_kib) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos, "max_dropped_events",
                                    thresholds->max_dropped_events) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos,
                                    "expected_state_checksum",
                                    thresholds->expected_state_checksum) &&
       capy_benchmark_append_kv_u32(out, out_size, &pos,
                                    "require_state_checksum",
                                    thresholds->require_state_checksum) &&
       capy_benchmark_append(out, out_size, &pos, "---\n");
  if (!ok) {
    out[0] = '\0';
    return 0;
  }
  out[pos] = '\0';
  return (int)pos;
}

/* Read side of the line-oriented key=value form. The parsing helpers below
 * scan a bounded [text, text+len) buffer without ever reading past len and
 * are the inverse of the append_* serialisation helpers above. They are pure
 * functions of the input bytes (no clocks, no allocation, no globals). */

static int capy_benchmark_match(const char *text, size_t len, size_t *pos,
                                const char *lit) {
  size_t p;
  size_t i = 0u;
  if (!text || !pos || !lit) {
    return 0;
  }
  p = *pos;
  while (lit[i]) {
    if (p >= len || text[p] != lit[i]) {
      return 0;
    }
    ++p;
    ++i;
  }
  *pos = p;
  return 1;
}

static int capy_benchmark_parse_u32(const char *text, size_t len, size_t *pos,
                                    uint32_t *out_val) {
  size_t p;
  size_t start;
  size_t digits = 0u;
  uint32_t value = 0u;
  if (!text || !pos || !out_val) {
    return 0;
  }
  p = *pos;
  start = p;
  while (p < len) {
    char ch = text[p];
    uint32_t digit;
    if (ch < '0' || ch > '9') {
      break;
    }
    digit = (uint32_t)(ch - '0');
    if (value > 0xFFFFFFFFu / 10u ||
        (value == 0xFFFFFFFFu / 10u && digit > 0xFFFFFFFFu % 10u)) {
      return 0; /* would overflow uint32_t */
    }
    value = value * 10u + digit;
    ++p;
    ++digits;
  }
  if (digits == 0u) {
    return 0; /* empty numeric field */
  }
  if (digits > 1u && text[start] == '0') {
    return 0; /* non-canonical leading zero */
  }
  *out_val = value;
  *pos = p;
  return 1;
}

static int capy_benchmark_parse_kv_u32(const char *text, size_t len,
                                       size_t *pos, const char *key,
                                       uint32_t *out_val) {
  return capy_benchmark_match(text, len, pos, key) &&
         capy_benchmark_match(text, len, pos, "=") &&
         capy_benchmark_parse_u32(text, len, pos, out_val) &&
         capy_benchmark_match(text, len, pos, "\n");
}

/* Parse `key=<value>\n`, copying the printable-ASCII value into `out`
 * (NUL-terminated). Inverse of capy_benchmark_append_kv_str: rejects an
 * empty value, any byte outside 0x20..0x7E, and a value that does not fit
 * `out_size` (so a report field can never be over-long). Consumes the
 * trailing newline on success. */
static int capy_benchmark_parse_kv_str(const char *text, size_t len,
                                       size_t *pos, const char *key, char *out,
                                       size_t out_size) {
  size_t p;
  size_t n = 0u;
  if (!text || !pos || !key || !out || out_size == 0u) {
    return 0;
  }
  out[0] = '\0';
  if (!capy_benchmark_match(text, len, pos, key) ||
      !capy_benchmark_match(text, len, pos, "=")) {
    return 0;
  }
  p = *pos;
  while (p < len && text[p] != '\n') {
    int c = (int)(unsigned char)text[p];
    if (c < 0x20 || c > 0x7E) {
      return 0; /* non-printable byte in value */
    }
    if (n + 1u >= out_size) {
      return 0; /* value too long for the destination field */
    }
    out[n++] = text[p];
    ++p;
  }
  if (n == 0u) {
    return 0; /* empty value: the serialiser never emits one */
  }
  if (p >= len || text[p] != '\n') {
    return 0; /* missing line terminator */
  }
  out[n] = '\0';
  *pos = p + 1u;
  return 1;
}

int capy_benchmark_replay_parse(const char *text, size_t len,
                                struct capy_benchmark_replay *out) {
  size_t pos = 0u;
  struct capy_benchmark_replay tmp;
  if (!out) {
    return 0;
  }
  capy_benchmark_zero(out, sizeof(*out));
  if (!text) {
    return 0;
  }
  capy_benchmark_zero(&tmp, sizeof(tmp));
  if (!capy_benchmark_parse_kv_u32(text, len, &pos, "replay_id",
                                   &tmp.replay_id) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "seed", &tmp.seed) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "frame_budget",
                                   &tmp.frame_budget) ||
      !capy_benchmark_match(text, len, &pos, "---\n")) {
    return 0;
  }
  if (pos != len) {
    return 0; /* trailing bytes: not the canonical form */
  }
  if (!capy_benchmark_replay_valid(&tmp)) {
    return 0; /* fail-closed (e.g. zero frame budget) */
  }
  *out = tmp;
  return 1;
}

int capy_benchmark_report_parse(const char *text, size_t len,
                                struct capy_benchmark_report *out) {
  size_t pos = 0u;
  struct capy_benchmark_report tmp;
  if (!out) {
    return 0;
  }
  capy_benchmark_zero(out, sizeof(*out));
  if (!text) {
    return 0;
  }
  capy_benchmark_zero(&tmp, sizeof(tmp));
  if (!capy_benchmark_parse_kv_str(text, len, &pos, "name", tmp.name,
                                   sizeof(tmp.name)) ||
      !capy_benchmark_parse_kv_str(text, len, &pos, "benchmark_version",
                                   tmp.benchmark_version,
                                   sizeof(tmp.benchmark_version)) ||
      !capy_benchmark_parse_kv_str(text, len, &pos, "runner_version",
                                   tmp.runner_version,
                                   sizeof(tmp.runner_version)) ||
      !capy_benchmark_parse_kv_str(text, len, &pos, "platform", tmp.platform,
                                   sizeof(tmp.platform)) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "replay_id",
                                   &tmp.replay_id) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "seed", &tmp.seed) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "average_fps_milli",
                                   &tmp.metrics.average_fps_milli) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "p95_frame_time_us",
                                   &tmp.metrics.p95_frame_time_us) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "p99_frame_time_us",
                                   &tmp.metrics.p99_frame_time_us) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "input_latency_us",
                                   &tmp.metrics.input_latency_us) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "cpu_usage_milli",
                                   &tmp.metrics.cpu_usage_milli) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "memory_peak_kib",
                                   &tmp.metrics.memory_peak_kib) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "dropped_events",
                                   &tmp.metrics.dropped_events) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "state_checksum",
                                   &tmp.metrics.state_checksum) ||
      !capy_benchmark_match(text, len, &pos, "---\n")) {
    return 0;
  }
  if (pos != len) {
    return 0; /* trailing bytes: not the canonical form */
  }
  if (!capy_benchmark_report_valid(&tmp)) {
    return 0; /* fail-closed (empty field or zero fps/p95/p99) */
  }
  *out = tmp;
  return 1;
}

/* Compare two NUL-terminated strings for exact equality without pulling in
 * <string.h> (the harness stays freestanding-friendly). */
static int capy_benchmark_streq(const char *a, const char *b) {
  size_t i = 0u;
  while (a[i] != '\0' && a[i] == b[i]) {
    ++i;
  }
  return a[i] == b[i];
}

/* Inverse of capy_benchmark_result_name: map a result token back to its code.
 * Returns 0 for any token the serialiser would never emit. */
static int capy_benchmark_result_from_name(
    const char *name, enum capy_benchmark_result_code *out) {
  if (capy_benchmark_streq(name, "pass")) {
    *out = CAPY_BENCHMARK_PASS;
    return 1;
  }
  if (capy_benchmark_streq(name, "regression")) {
    *out = CAPY_BENCHMARK_FAIL_REGRESSION;
    return 1;
  }
  if (capy_benchmark_streq(name, "invalid_report")) {
    *out = CAPY_BENCHMARK_FAIL_INVALID_REPORT;
    return 1;
  }
  if (capy_benchmark_streq(name, "unsupported")) {
    *out = CAPY_BENCHMARK_FAIL_UNSUPPORTED;
    return 1;
  }
  return 0;
}

int capy_benchmark_evaluation_parse(const char *text, size_t len,
                                    struct capy_benchmark_evaluation *out) {
  size_t pos = 0u;
  struct capy_benchmark_evaluation tmp;
  char result[16];
  if (!out) {
    return 0;
  }
  capy_benchmark_zero(out, sizeof(*out));
  if (!text) {
    return 0;
  }
  capy_benchmark_zero(&tmp, sizeof(tmp));
  if (!capy_benchmark_parse_kv_str(text, len, &pos, "result", result,
                                   sizeof(result)) ||
      !capy_benchmark_parse_kv_str(text, len, &pos, "reason", tmp.reason,
                                   sizeof(tmp.reason)) ||
      !capy_benchmark_match(text, len, &pos, "---\n")) {
    return 0;
  }
  if (pos != len) {
    return 0; /* trailing bytes: not the canonical form */
  }
  if (!capy_benchmark_result_from_name(result, &tmp.code)) {
    return 0; /* fail-closed (unknown result token) */
  }
  *out = tmp;
  return 1;
}

int capy_benchmark_thresholds_parse(const char *text, size_t len,
                                    struct capy_benchmark_thresholds *out) {
  size_t pos = 0u;
  struct capy_benchmark_thresholds tmp;
  if (!out) {
    return 0;
  }
  capy_benchmark_zero(out, sizeof(*out));
  if (!text) {
    return 0;
  }
  capy_benchmark_zero(&tmp, sizeof(tmp));
  if (!capy_benchmark_parse_kv_u32(text, len, &pos, "min_average_fps_milli",
                                   &tmp.min_average_fps_milli) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "max_p95_frame_time_us",
                                   &tmp.max_p95_frame_time_us) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "max_p99_frame_time_us",
                                   &tmp.max_p99_frame_time_us) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "max_input_latency_us",
                                   &tmp.max_input_latency_us) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "max_cpu_usage_milli",
                                   &tmp.max_cpu_usage_milli) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "max_memory_peak_kib",
                                   &tmp.max_memory_peak_kib) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "max_dropped_events",
                                   &tmp.max_dropped_events) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "expected_state_checksum",
                                   &tmp.expected_state_checksum) ||
      !capy_benchmark_parse_kv_u32(text, len, &pos, "require_state_checksum",
                                   &tmp.require_state_checksum) ||
      !capy_benchmark_match(text, len, &pos, "---\n")) {
    return 0;
  }
  if (pos != len) {
    return 0; /* trailing bytes: not the canonical form */
  }
  /* every threshold combination is valid (0 = "no bound" per evaluate), so
   * any canonical byte sequence round-trips without a semantic gate. */
  *out = tmp;
  return 1;
}
