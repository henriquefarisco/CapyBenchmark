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

static void test_serialize_is_deterministic(void) {
  struct capy_benchmark_report report = valid_report();
  char a[CAPY_BENCHMARK_SERIAL_MAX];
  char b[CAPY_BENCHMARK_SERIAL_MAX];
  int la = capy_benchmark_report_serialize(&report, a, sizeof(a));
  int lb = capy_benchmark_report_serialize(&report, b, sizeof(b));
  EXPECT(la > 0);
  EXPECT(la == lb);
  EXPECT(strcmp(a, b) == 0);
}

static void test_serialize_matches_golden(void) {
  struct capy_benchmark_report report = valid_report();
  char out[CAPY_BENCHMARK_SERIAL_MAX];
  static const char expected[] =
      "name=snake-frame\n"
      "benchmark_version=0.0.1\n"
      "runner_version=0.0.1\n"
      "platform=host\n"
      "replay_id=7\n"
      "seed=42\n"
      "average_fps_milli=60000\n"
      "p95_frame_time_us=17000\n"
      "p99_frame_time_us=18000\n"
      "input_latency_us=4000\n"
      "cpu_usage_milli=250\n"
      "memory_peak_kib=4096\n"
      "dropped_events=0\n"
      "state_checksum=12648430\n"
      "---\n";
  int n = capy_benchmark_report_serialize(&report, out, sizeof(out));
  EXPECT(n > 0);
  EXPECT((size_t)n == strlen(expected));
  EXPECT(strcmp(out, expected) == 0);
}

static void test_serialize_rejects_invalid_report(void) {
  struct capy_benchmark_report report = valid_report();
  char out[CAPY_BENCHMARK_SERIAL_MAX];
  report.metrics.average_fps_milli = 0u;
  out[0] = 'X';
  EXPECT(capy_benchmark_report_serialize(&report, out, sizeof(out)) == 0);
  EXPECT(out[0] == '\0');
}

static void test_serialize_fails_closed_on_small_buffer(void) {
  struct capy_benchmark_report report = valid_report();
  char out[8];
  out[0] = 'X';
  EXPECT(capy_benchmark_report_serialize(&report, out, sizeof(out)) == 0);
  EXPECT(out[0] == '\0');
}

static void test_evaluation_serialize_pass_golden(void) {
  struct capy_benchmark_report report = valid_report();
  struct capy_benchmark_thresholds thresholds;
  struct capy_benchmark_evaluation eval;
  char out[CAPY_BENCHMARK_SERIAL_MAX];
  static const char expected[] =
      "result=pass\n"
      "reason=pass\n"
      "---\n";
  memset(&thresholds, 0, sizeof(thresholds));
  capy_benchmark_evaluate(&report, &thresholds, &eval);
  EXPECT(eval.code == CAPY_BENCHMARK_PASS);
  EXPECT(capy_benchmark_evaluation_serialize(&eval, out, sizeof(out)) > 0);
  EXPECT(strcmp(out, expected) == 0);
}

static void test_evaluation_serialize_regression_golden(void) {
  struct capy_benchmark_report report = valid_report();
  struct capy_benchmark_thresholds thresholds;
  struct capy_benchmark_evaluation eval;
  char out[CAPY_BENCHMARK_SERIAL_MAX];
  static const char expected[] =
      "result=regression\n"
      "reason=average fps below baseline\n"
      "---\n";
  memset(&thresholds, 0, sizeof(thresholds));
  thresholds.min_average_fps_milli = 120000u;
  capy_benchmark_evaluate(&report, &thresholds, &eval);
  EXPECT(eval.code == CAPY_BENCHMARK_FAIL_REGRESSION);
  EXPECT(capy_benchmark_evaluation_serialize(&eval, out, sizeof(out)) > 0);
  EXPECT(strcmp(out, expected) == 0);
}

static void test_evaluation_serialize_is_deterministic(void) {
  struct capy_benchmark_report report = valid_report();
  struct capy_benchmark_thresholds thresholds;
  struct capy_benchmark_evaluation eval;
  char a[CAPY_BENCHMARK_SERIAL_MAX];
  char b[CAPY_BENCHMARK_SERIAL_MAX];
  int la, lb;
  memset(&thresholds, 0, sizeof(thresholds));
  capy_benchmark_evaluate(&report, &thresholds, &eval);
  la = capy_benchmark_evaluation_serialize(&eval, a, sizeof(a));
  lb = capy_benchmark_evaluation_serialize(&eval, b, sizeof(b));
  EXPECT(la > 0);
  EXPECT(la == lb);
  EXPECT(strcmp(a, b) == 0);
}

static void test_evaluation_serialize_rejects_unknown_code(void) {
  struct capy_benchmark_report report = valid_report();
  struct capy_benchmark_thresholds thresholds;
  struct capy_benchmark_evaluation eval;
  char out[CAPY_BENCHMARK_SERIAL_MAX];
  memset(&thresholds, 0, sizeof(thresholds));
  capy_benchmark_evaluate(&report, &thresholds, &eval);
  eval.code = (enum capy_benchmark_result_code)42;
  out[0] = 'X';
  EXPECT(capy_benchmark_evaluation_serialize(&eval, out, sizeof(out)) == 0);
  EXPECT(out[0] == '\0');
}

static void test_evaluation_serialize_fails_closed_on_small_buffer(void) {
  struct capy_benchmark_report report = valid_report();
  struct capy_benchmark_thresholds thresholds;
  struct capy_benchmark_evaluation eval;
  char out[8];
  memset(&thresholds, 0, sizeof(thresholds));
  capy_benchmark_evaluate(&report, &thresholds, &eval);
  out[0] = 'X';
  EXPECT(capy_benchmark_evaluation_serialize(&eval, out, sizeof(out)) == 0);
  EXPECT(out[0] == '\0');
}

static struct capy_benchmark_report snake_host_report(void) {
  struct capy_benchmark_report report;
  capy_benchmark_report_init(&report, "snake-frame", "1.0.0", "0.0.6", "host");
  report.replay_id = 1u;
  report.seed = 1337u;
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

static struct capy_benchmark_thresholds snake_host_baseline(void) {
  struct capy_benchmark_thresholds t;
  memset(&t, 0, sizeof(t));
  t.min_average_fps_milli = 55000u;
  t.max_p95_frame_time_us = 18000u;
  t.max_p99_frame_time_us = 20000u;
  t.max_input_latency_us = 6000u;
  t.max_cpu_usage_milli = 400u;
  t.max_memory_peak_kib = 8192u;
  t.max_dropped_events = 0u;
  t.expected_state_checksum = 0xC0FFEEu;
  t.require_state_checksum = 1u;
  return t;
}

static struct capy_benchmark_report asteroids_host_report(void) {
  struct capy_benchmark_report report;
  capy_benchmark_report_init(&report, "asteroids-frame", "1.0.0", "0.0.6",
                             "host");
  report.replay_id = 2u;
  report.seed = 2024u;
  report.metrics.average_fps_milli = 48000u;
  report.metrics.p95_frame_time_us = 22000u;
  report.metrics.p99_frame_time_us = 26000u;
  report.metrics.input_latency_us = 5000u;
  report.metrics.cpu_usage_milli = 380u;
  report.metrics.memory_peak_kib = 6144u;
  report.metrics.dropped_events = 1u;
  report.metrics.state_checksum = 0xA57Eu;
  return report;
}

static struct capy_benchmark_thresholds asteroids_host_baseline(void) {
  struct capy_benchmark_thresholds t;
  memset(&t, 0, sizeof(t));
  t.min_average_fps_milli = 45000u;
  t.max_p95_frame_time_us = 24000u;
  t.max_p99_frame_time_us = 28000u;
  t.max_input_latency_us = 7000u;
  t.max_cpu_usage_milli = 500u;
  t.max_memory_peak_kib = 8192u;
  t.max_dropped_events = 2u;
  t.expected_state_checksum = 0xA57Eu;
  t.require_state_checksum = 1u;
  return t;
}

static void test_snake_host_baseline_passes(void) {
  struct capy_benchmark_report report = snake_host_report();
  struct capy_benchmark_thresholds baseline = snake_host_baseline();
  struct capy_benchmark_evaluation eval;
  EXPECT(capy_benchmark_report_valid(&report) == 1);
  capy_benchmark_evaluate(&report, &baseline, &eval);
  EXPECT(eval.code == CAPY_BENCHMARK_PASS);
  EXPECT(strcmp(eval.reason, "pass") == 0);
}

static void test_asteroids_host_baseline_passes(void) {
  struct capy_benchmark_report report = asteroids_host_report();
  struct capy_benchmark_thresholds baseline = asteroids_host_baseline();
  struct capy_benchmark_evaluation eval;
  EXPECT(capy_benchmark_report_valid(&report) == 1);
  capy_benchmark_evaluate(&report, &baseline, &eval);
  EXPECT(eval.code == CAPY_BENCHMARK_PASS);
}

static void test_snake_fps_regression_fails_closed(void) {
  struct capy_benchmark_report report = snake_host_report();
  struct capy_benchmark_thresholds baseline = snake_host_baseline();
  struct capy_benchmark_evaluation eval;
  report.metrics.average_fps_milli = 40000u;
  capy_benchmark_evaluate(&report, &baseline, &eval);
  EXPECT(eval.code == CAPY_BENCHMARK_FAIL_REGRESSION);
  EXPECT(strcmp(eval.reason, "average fps below baseline") == 0);
}

static void test_snake_checksum_mismatch_fails_closed(void) {
  struct capy_benchmark_report report = snake_host_report();
  struct capy_benchmark_thresholds baseline = snake_host_baseline();
  struct capy_benchmark_evaluation eval;
  report.metrics.state_checksum = 0xDEADBEEFu;
  capy_benchmark_evaluate(&report, &baseline, &eval);
  EXPECT(eval.code == CAPY_BENCHMARK_FAIL_REGRESSION);
  EXPECT(strcmp(eval.reason, "state checksum mismatch") == 0);
}

static void test_baseline_tolerance_varies_by_platform(void) {
  struct capy_benchmark_report report = snake_host_report();
  struct capy_benchmark_thresholds strict = snake_host_baseline();
  struct capy_benchmark_thresholds vm;
  struct capy_benchmark_evaluation eval_strict;
  struct capy_benchmark_evaluation eval_vm;
  report.metrics.average_fps_milli = 50000u;
  memset(&vm, 0, sizeof(vm));
  vm.min_average_fps_milli = 45000u;
  vm.expected_state_checksum = 0xC0FFEEu;
  vm.require_state_checksum = 1u;
  strict.min_average_fps_milli = 58000u;
  capy_benchmark_evaluate(&report, &strict, &eval_strict);
  capy_benchmark_evaluate(&report, &vm, &eval_vm);
  EXPECT(eval_strict.code == CAPY_BENCHMARK_FAIL_REGRESSION);
  EXPECT(eval_vm.code == CAPY_BENCHMARK_PASS);
}

static void test_baseline_verdict_is_deterministic(void) {
  struct capy_benchmark_report report = asteroids_host_report();
  struct capy_benchmark_thresholds baseline = asteroids_host_baseline();
  struct capy_benchmark_evaluation a;
  struct capy_benchmark_evaluation b;
  capy_benchmark_evaluate(&report, &baseline, &a);
  capy_benchmark_evaluate(&report, &baseline, &b);
  EXPECT(a.code == b.code);
  EXPECT(strcmp(a.reason, b.reason) == 0);
}

static void test_golden_serialized_snake_report(void) {
  struct capy_benchmark_report report = snake_host_report();
  char out[CAPY_BENCHMARK_SERIAL_MAX];
  static const char expected[] =
      "name=snake-frame\n"
      "benchmark_version=1.0.0\n"
      "runner_version=0.0.6\n"
      "platform=host\n"
      "replay_id=1\n"
      "seed=1337\n"
      "average_fps_milli=60000\n"
      "p95_frame_time_us=17000\n"
      "p99_frame_time_us=18000\n"
      "input_latency_us=4000\n"
      "cpu_usage_milli=250\n"
      "memory_peak_kib=4096\n"
      "dropped_events=0\n"
      "state_checksum=12648430\n"
      "---\n";
  EXPECT(capy_benchmark_report_serialize(&report, out, sizeof(out)) > 0);
  EXPECT(strcmp(out, expected) == 0);
}

static void test_golden_serialized_asteroids_report(void) {
  struct capy_benchmark_report report = asteroids_host_report();
  char out[CAPY_BENCHMARK_SERIAL_MAX];
  static const char expected[] =
      "name=asteroids-frame\n"
      "benchmark_version=1.0.0\n"
      "runner_version=0.0.6\n"
      "platform=host\n"
      "replay_id=2\n"
      "seed=2024\n"
      "average_fps_milli=48000\n"
      "p95_frame_time_us=22000\n"
      "p99_frame_time_us=26000\n"
      "input_latency_us=5000\n"
      "cpu_usage_milli=380\n"
      "memory_peak_kib=6144\n"
      "dropped_events=1\n"
      "state_checksum=42366\n"
      "---\n";
  EXPECT(capy_benchmark_report_serialize(&report, out, sizeof(out)) > 0);
  EXPECT(strcmp(out, expected) == 0);
}

static struct capy_benchmark_replay snake_replay(void) {
  struct capy_benchmark_replay replay;
  replay.replay_id = 7u;
  replay.seed = 42u;
  replay.frame_budget = 600u;
  return replay;
}

static void test_replay_valid_serializes_golden(void) {
  struct capy_benchmark_replay replay = snake_replay();
  char out[CAPY_BENCHMARK_SERIAL_MAX];
  static const char expected[] =
      "replay_id=7\n"
      "seed=42\n"
      "frame_budget=600\n"
      "---\n";
  EXPECT(capy_benchmark_replay_valid(&replay) == 1);
  EXPECT(capy_benchmark_replay_serialize(&replay, out, sizeof(out)) > 0);
  EXPECT(strcmp(out, expected) == 0);
}

static void test_replay_serialize_is_deterministic(void) {
  struct capy_benchmark_replay replay = snake_replay();
  char a[CAPY_BENCHMARK_SERIAL_MAX];
  char b[CAPY_BENCHMARK_SERIAL_MAX];
  int la = capy_benchmark_replay_serialize(&replay, a, sizeof(a));
  int lb = capy_benchmark_replay_serialize(&replay, b, sizeof(b));
  EXPECT(la > 0);
  EXPECT(la == lb);
  EXPECT(strcmp(a, b) == 0);
}

static void test_replay_rejects_zero_frame_budget(void) {
  struct capy_benchmark_replay replay = snake_replay();
  char out[CAPY_BENCHMARK_SERIAL_MAX];
  replay.frame_budget = 0u;
  out[0] = 'X';
  EXPECT(capy_benchmark_replay_valid(&replay) == 0);
  EXPECT(capy_benchmark_replay_serialize(&replay, out, sizeof(out)) == 0);
  EXPECT(out[0] == '\0');
}

static void test_replay_serialize_fails_closed_on_small_buffer(void) {
  struct capy_benchmark_replay replay = snake_replay();
  char out[8];
  out[0] = 'X';
  EXPECT(capy_benchmark_replay_serialize(&replay, out, sizeof(out)) == 0);
  EXPECT(out[0] == '\0');
}

static void test_report_init_truncates_long_name(void) {
  struct capy_benchmark_report report;
  char long_name[CAPY_BENCHMARK_NAME_MAX + 16];
  size_t i;
  for (i = 0u; i < sizeof(long_name) - 1u; ++i) {
    long_name[i] = 'a';
  }
  long_name[sizeof(long_name) - 1u] = '\0';
  capy_benchmark_report_init(&report, long_name, "1.0.0", "0.0.6", "host");
  EXPECT(strlen(report.name) == CAPY_BENCHMARK_NAME_MAX - 1u);
  report.metrics.average_fps_milli = 60000u;
  report.metrics.p95_frame_time_us = 17000u;
  report.metrics.p99_frame_time_us = 18000u;
  EXPECT(capy_benchmark_report_valid(&report) == 1);
}

static void test_serialize_rejects_non_printable_field(void) {
  struct capy_benchmark_report report = valid_report();
  char out[CAPY_BENCHMARK_SERIAL_MAX];
  report.name[0] = (char)0x09;
  out[0] = 'X';
  EXPECT(capy_benchmark_report_serialize(&report, out, sizeof(out)) == 0);
  EXPECT(out[0] == '\0');
}

static void test_serialize_accepts_printable_boundaries(void) {
  struct capy_benchmark_report report;
  char out[CAPY_BENCHMARK_SERIAL_MAX];
  capy_benchmark_report_init(&report, "snake-frame", "1.0.0", "0.0.6", "a ~");
  report.metrics.average_fps_milli = 60000u;
  report.metrics.p95_frame_time_us = 17000u;
  report.metrics.p99_frame_time_us = 18000u;
  EXPECT(capy_benchmark_report_serialize(&report, out, sizeof(out)) > 0);
}

static void test_serialize_handles_max_uint32_metric(void) {
  struct capy_benchmark_report report = valid_report();
  char out[CAPY_BENCHMARK_SERIAL_MAX];
  static const char needle[] = "memory_peak_kib=4294967295\n";
  report.metrics.memory_peak_kib = 4294967295u;
  EXPECT(capy_benchmark_report_serialize(&report, out, sizeof(out)) > 0);
  EXPECT(strstr(out, needle) != 0);
}

static void test_serialize_buffer_boundary_is_exact(void) {
  struct capy_benchmark_report report = valid_report();
  char big[CAPY_BENCHMARK_SERIAL_MAX];
  char exact[CAPY_BENCHMARK_SERIAL_MAX];
  char tight[CAPY_BENCHMARK_SERIAL_MAX];
  int n;
  int ne;
  n = capy_benchmark_report_serialize(&report, big, sizeof(big));
  EXPECT(n > 0);
  ne = capy_benchmark_report_serialize(&report, exact, (size_t)n + 1u);
  EXPECT(ne == n);
  EXPECT(strcmp(exact, big) == 0);
  tight[0] = 'X';
  EXPECT(capy_benchmark_report_serialize(&report, tight, (size_t)n) == 0);
  EXPECT(tight[0] == '\0');
}

static void test_report_valid_rejects_empty_name(void) {
  struct capy_benchmark_report report = valid_report();
  char out[CAPY_BENCHMARK_SERIAL_MAX];
  report.name[0] = '\0';
  EXPECT(capy_benchmark_report_valid(&report) == 0);
  out[0] = 'X';
  EXPECT(capy_benchmark_report_serialize(&report, out, sizeof(out)) == 0);
  EXPECT(out[0] == '\0');
}

static void test_report_valid_rejects_zero_frame_times(void) {
  struct capy_benchmark_report a = valid_report();
  struct capy_benchmark_report b = valid_report();
  a.metrics.p95_frame_time_us = 0u;
  b.metrics.p99_frame_time_us = 0u;
  EXPECT(capy_benchmark_report_valid(&a) == 0);
  EXPECT(capy_benchmark_report_valid(&b) == 0);
}

static void test_checksum_gate_accepts_zero_when_required(void) {
  struct capy_benchmark_report report = valid_report();
  struct capy_benchmark_thresholds t;
  struct capy_benchmark_evaluation eval;
  memset(&t, 0, sizeof(t));
  report.metrics.state_checksum = 0u;
  t.require_state_checksum = 1u;
  t.expected_state_checksum = 0u;
  capy_benchmark_evaluate(&report, &t, &eval);
  EXPECT(eval.code == CAPY_BENCHMARK_PASS);
}

int main(void) {
  test_valid_report_passes_thresholds();
  test_regression_fails_closed();
  test_invalid_report_is_rejected();
  test_serialize_is_deterministic();
  test_serialize_matches_golden();
  test_serialize_rejects_invalid_report();
  test_serialize_fails_closed_on_small_buffer();
  test_evaluation_serialize_pass_golden();
  test_evaluation_serialize_regression_golden();
  test_evaluation_serialize_is_deterministic();
  test_evaluation_serialize_rejects_unknown_code();
  test_evaluation_serialize_fails_closed_on_small_buffer();
  test_snake_host_baseline_passes();
  test_asteroids_host_baseline_passes();
  test_snake_fps_regression_fails_closed();
  test_snake_checksum_mismatch_fails_closed();
  test_baseline_tolerance_varies_by_platform();
  test_baseline_verdict_is_deterministic();
  test_golden_serialized_snake_report();
  test_golden_serialized_asteroids_report();
  test_replay_valid_serializes_golden();
  test_replay_serialize_is_deterministic();
  test_replay_rejects_zero_frame_budget();
  test_replay_serialize_fails_closed_on_small_buffer();
  test_report_init_truncates_long_name();
  test_serialize_rejects_non_printable_field();
  test_serialize_accepts_printable_boundaries();
  test_serialize_handles_max_uint32_metric();
  test_serialize_buffer_boundary_is_exact();
  test_report_valid_rejects_empty_name();
  test_report_valid_rejects_zero_frame_times();
  test_checksum_gate_accepts_zero_when_required();
  return failures == 0 ? 0 : 1;
}
