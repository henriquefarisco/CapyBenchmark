#include "capy_benchmark.h"

#include <string.h>

static int failures;

#define EXPECT(expr)      \
  do {                    \
    if (!(expr)) {        \
      ++failures;         \
      return;             \
    }                     \
  } while (0)

static struct capy_benchmark_report valid_report(void) {
  struct capy_benchmark_report report;
  capy_benchmark_report_init(&report, "snake-frame", "0.0.1", "0.0.1", "host");
  report.replay_id = 7u;
  report.seed = 42u;
  report.metrics.average_fps_milli = 60000u;
  report.metrics.p95_frame_time_us = 17000u;
  report.metrics.p99_frame_time_us = 18000u;
  report.metrics.input_latency_us = 4000u;
  report.metrics.cpu_usage_milli = 250u;
  report.metrics.memory_peak_kib = 4096u;
  report.metrics.dropped_events = 0u;
  report.metrics.state_checksum = 0xC0FFEEu;
  return report;
}

static void test_valid_report_passes_thresholds(void) {
  struct capy_benchmark_report report = valid_report();
  struct capy_benchmark_thresholds thresholds;
  struct capy_benchmark_evaluation eval;
  memset(&thresholds, 0, sizeof(thresholds));
  thresholds.min_average_fps_milli = 30000u;
  thresholds.max_p95_frame_time_us = 20000u;
  thresholds.max_p99_frame_time_us = 22000u;
  thresholds.max_input_latency_us = 8000u;
  thresholds.max_cpu_usage_milli = 500u;
  thresholds.max_memory_peak_kib = 8192u;
  thresholds.max_dropped_events = 0u;
  thresholds.expected_state_checksum = 0xC0FFEEu;
  thresholds.require_state_checksum = 1u;
  EXPECT(capy_benchmark_report_valid(&report) == 1);
  capy_benchmark_evaluate(&report, &thresholds, &eval);
  EXPECT(eval.code == CAPY_BENCHMARK_PASS);
  EXPECT(strcmp(eval.reason, "pass") == 0);
}

static void test_regression_fails_closed(void) {
  struct capy_benchmark_report report = valid_report();
  struct capy_benchmark_thresholds thresholds;
  struct capy_benchmark_evaluation eval;
  memset(&thresholds, 0, sizeof(thresholds));
  thresholds.min_average_fps_milli = 120000u;
  capy_benchmark_evaluate(&report, &thresholds, &eval);
  EXPECT(eval.code == CAPY_BENCHMARK_FAIL_REGRESSION);
}

static void test_invalid_report_is_rejected(void) {
  struct capy_benchmark_report report = valid_report();
  struct capy_benchmark_thresholds thresholds;
  struct capy_benchmark_evaluation eval;
  memset(&thresholds, 0, sizeof(thresholds));
  report.metrics.average_fps_milli = 0u;
  capy_benchmark_evaluate(&report, &thresholds, &eval);
  EXPECT(eval.code == CAPY_BENCHMARK_FAIL_INVALID_REPORT);
}

int main(void) {
  test_valid_report_passes_thresholds();
  test_regression_fails_closed();
  test_invalid_report_is_rejected();
  return failures == 0 ? 0 : 1;
}
