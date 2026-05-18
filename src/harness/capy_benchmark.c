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
