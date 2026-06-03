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
