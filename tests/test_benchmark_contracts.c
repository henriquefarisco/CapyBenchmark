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

static void test_replay_round_trips_through_parse(void) {
  struct capy_benchmark_replay replay = snake_replay();
  struct capy_benchmark_replay parsed;
  char buf[CAPY_BENCHMARK_SERIAL_MAX];
  char buf2[CAPY_BENCHMARK_SERIAL_MAX];
  int n = capy_benchmark_replay_serialize(&replay, buf, sizeof(buf));
  EXPECT(n > 0);
  EXPECT(capy_benchmark_replay_parse(buf, (size_t)n, &parsed) == 1);
  EXPECT(parsed.replay_id == replay.replay_id);
  EXPECT(parsed.seed == replay.seed);
  EXPECT(parsed.frame_budget == replay.frame_budget);
  /* re-serialising the parsed replay reproduces the bytes exactly */
  EXPECT(capy_benchmark_replay_serialize(&parsed, buf2, sizeof(buf2)) == n);
  EXPECT(strcmp(buf, buf2) == 0);
}

static void test_replay_parse_is_deterministic(void) {
  struct capy_benchmark_replay a;
  struct capy_benchmark_replay b;
  static const char text[] =
      "replay_id=7\n"
      "seed=42\n"
      "frame_budget=600\n"
      "---\n";
  EXPECT(capy_benchmark_replay_parse(text, sizeof(text) - 1u, &a) == 1);
  EXPECT(capy_benchmark_replay_parse(text, sizeof(text) - 1u, &b) == 1);
  EXPECT(a.replay_id == b.replay_id);
  EXPECT(a.seed == b.seed);
  EXPECT(a.frame_budget == b.frame_budget);
}

static void test_replay_parse_accepts_max_u32(void) {
  struct capy_benchmark_replay parsed;
  static const char text[] =
      "replay_id=4294967295\n"
      "seed=0\n"
      "frame_budget=1\n"
      "---\n";
  EXPECT(capy_benchmark_replay_parse(text, sizeof(text) - 1u, &parsed) == 1);
  EXPECT(parsed.replay_id == 4294967295u);
  EXPECT(parsed.seed == 0u);
  EXPECT(parsed.frame_budget == 1u);
}

static void test_replay_parse_rejects_malformed(void) {
  struct capy_benchmark_replay parsed;
  static const char no_sentinel[] =
      "replay_id=7\n"
      "seed=42\n"
      "frame_budget=600\n";
  static const char reordered[] =
      "seed=42\n"
      "replay_id=7\n"
      "frame_budget=600\n"
      "---\n";
  static const char unknown_key[] =
      "frames=7\n"
      "seed=42\n"
      "frame_budget=600\n"
      "---\n";
  static const char non_numeric[] =
      "replay_id=7x\n"
      "seed=42\n"
      "frame_budget=600\n"
      "---\n";
  static const char empty_value[] =
      "replay_id=\n"
      "seed=42\n"
      "frame_budget=600\n"
      "---\n";
  static const char leading_zero[] =
      "replay_id=07\n"
      "seed=42\n"
      "frame_budget=600\n"
      "---\n";
  static const char overflow[] =
      "replay_id=4294967296\n"
      "seed=42\n"
      "frame_budget=600\n"
      "---\n";
  static const char trailing[] =
      "replay_id=7\n"
      "seed=42\n"
      "frame_budget=600\n"
      "---\nextra";
  static const char zero_budget[] =
      "replay_id=7\n"
      "seed=42\n"
      "frame_budget=0\n"
      "---\n";
  parsed.frame_budget = 0xFFu;
  EXPECT(capy_benchmark_replay_parse(no_sentinel, sizeof(no_sentinel) - 1u,
                                     &parsed) == 0);
  EXPECT(parsed.frame_budget == 0u); /* out is zeroed on failure */
  EXPECT(capy_benchmark_replay_parse(reordered, sizeof(reordered) - 1u,
                                     &parsed) == 0);
  EXPECT(capy_benchmark_replay_parse(unknown_key, sizeof(unknown_key) - 1u,
                                     &parsed) == 0);
  EXPECT(capy_benchmark_replay_parse(non_numeric, sizeof(non_numeric) - 1u,
                                     &parsed) == 0);
  EXPECT(capy_benchmark_replay_parse(empty_value, sizeof(empty_value) - 1u,
                                     &parsed) == 0);
  EXPECT(capy_benchmark_replay_parse(leading_zero, sizeof(leading_zero) - 1u,
                                     &parsed) == 0);
  EXPECT(capy_benchmark_replay_parse(overflow, sizeof(overflow) - 1u,
                                     &parsed) == 0);
  EXPECT(capy_benchmark_replay_parse(trailing, sizeof(trailing) - 1u,
                                     &parsed) == 0);
  EXPECT(capy_benchmark_replay_parse(zero_budget, sizeof(zero_budget) - 1u,
                                     &parsed) == 0);
}

static void test_replay_parse_rejects_null_text(void) {
  struct capy_benchmark_replay parsed;
  parsed.frame_budget = 0xFFu;
  EXPECT(capy_benchmark_replay_parse(NULL, 0u, &parsed) == 0);
  EXPECT(parsed.frame_budget == 0u);
}

static void test_report_round_trips_through_parse(void) {
  struct capy_benchmark_report report = valid_report();
  struct capy_benchmark_report parsed;
  char buf[CAPY_BENCHMARK_SERIAL_MAX];
  char buf2[CAPY_BENCHMARK_SERIAL_MAX];
  int n = capy_benchmark_report_serialize(&report, buf, sizeof(buf));
  EXPECT(n > 0);
  EXPECT(capy_benchmark_report_parse(buf, (size_t)n, &parsed) == 1);
  EXPECT(strcmp(parsed.name, report.name) == 0);
  EXPECT(strcmp(parsed.benchmark_version, report.benchmark_version) == 0);
  EXPECT(strcmp(parsed.runner_version, report.runner_version) == 0);
  EXPECT(strcmp(parsed.platform, report.platform) == 0);
  EXPECT(parsed.replay_id == report.replay_id);
  EXPECT(parsed.seed == report.seed);
  EXPECT(parsed.metrics.average_fps_milli == report.metrics.average_fps_milli);
  EXPECT(parsed.metrics.p95_frame_time_us == report.metrics.p95_frame_time_us);
  EXPECT(parsed.metrics.p99_frame_time_us == report.metrics.p99_frame_time_us);
  EXPECT(parsed.metrics.input_latency_us == report.metrics.input_latency_us);
  EXPECT(parsed.metrics.cpu_usage_milli == report.metrics.cpu_usage_milli);
  EXPECT(parsed.metrics.memory_peak_kib == report.metrics.memory_peak_kib);
  EXPECT(parsed.metrics.dropped_events == report.metrics.dropped_events);
  EXPECT(parsed.metrics.state_checksum == report.metrics.state_checksum);
  /* re-serialising the parsed report reproduces the bytes exactly */
  EXPECT(capy_benchmark_report_serialize(&parsed, buf2, sizeof(buf2)) == n);
  EXPECT(strcmp(buf, buf2) == 0);
}

static void test_report_parse_rejects_malformed(void) {
  struct capy_benchmark_report report = valid_report();
  struct capy_benchmark_report parsed;
  char canon[CAPY_BENCHMARK_SERIAL_MAX];
  char buf[CAPY_BENCHMARK_SERIAL_MAX];
  size_t i;
  int n = capy_benchmark_report_serialize(&report, canon, sizeof(canon));
  static const char unknown_key[] = "label=snake-frame\n---\n";
  static const char empty_name[] = "name=\n---\n";
  static const char nonprintable[] = "name=a\001b\n---\n";
  EXPECT(n > 0);
  EXPECT(capy_benchmark_report_parse(canon, (size_t)n, &parsed) == 1);
  /* trailing byte after the sentinel is rejected */
  memcpy(buf, canon, (size_t)n);
  buf[n] = 'x';
  EXPECT(capy_benchmark_report_parse(buf, (size_t)n + 1u, &parsed) == 0);
  /* missing sentinel (drop the trailing "---\n"); out is zeroed on failure */
  parsed.replay_id = 0xABCDu;
  EXPECT(capy_benchmark_report_parse(canon, (size_t)n - 4u, &parsed) == 0);
  EXPECT(parsed.replay_id == 0u);
  /* report-specific field failures: unknown first key, empty + non-printable
     string values */
  EXPECT(capy_benchmark_report_parse(unknown_key, sizeof(unknown_key) - 1u,
                                     &parsed) == 0);
  EXPECT(capy_benchmark_report_parse(empty_name, sizeof(empty_name) - 1u,
                                     &parsed) == 0);
  EXPECT(capy_benchmark_report_parse(nonprintable, sizeof(nonprintable) - 1u,
                                     &parsed) == 0);
  /* a value longer than the destination field cap is rejected */
  memcpy(buf, "name=", 5u);
  for (i = 0u; i < CAPY_BENCHMARK_NAME_MAX + 4u; ++i) {
    buf[5u + i] = 'x';
  }
  buf[5u + i] = '\n';
  EXPECT(capy_benchmark_report_parse(buf, 5u + i + 1u, &parsed) == 0);
}

static void test_report_parse_rejects_null_text(void) {
  struct capy_benchmark_report parsed;
  parsed.replay_id = 0xABCDu;
  EXPECT(capy_benchmark_report_parse(NULL, 0u, &parsed) == 0);
  EXPECT(parsed.replay_id == 0u);
}

static void test_evaluation_round_trips_through_parse(void) {
  struct capy_benchmark_report report = valid_report();
  struct capy_benchmark_thresholds thresholds;
  struct capy_benchmark_evaluation eval;
  struct capy_benchmark_evaluation parsed;
  char buf[CAPY_BENCHMARK_SERIAL_MAX];
  char buf2[CAPY_BENCHMARK_SERIAL_MAX];
  int n;
  memset(&thresholds, 0, sizeof(thresholds));
  thresholds.min_average_fps_milli = 120000u; /* force a regression verdict */
  capy_benchmark_evaluate(&report, &thresholds, &eval);
  EXPECT(eval.code == CAPY_BENCHMARK_FAIL_REGRESSION);
  n = capy_benchmark_evaluation_serialize(&eval, buf, sizeof(buf));
  EXPECT(n > 0);
  EXPECT(capy_benchmark_evaluation_parse(buf, (size_t)n, &parsed) == 1);
  EXPECT(parsed.code == eval.code);
  EXPECT(strcmp(parsed.reason, eval.reason) == 0);
  /* re-serialising the parsed evaluation reproduces the bytes exactly */
  EXPECT(capy_benchmark_evaluation_serialize(&parsed, buf2, sizeof(buf2)) == n);
  EXPECT(strcmp(buf, buf2) == 0);
}

static void test_evaluation_parse_maps_all_result_tokens(void) {
  struct capy_benchmark_evaluation parsed;
  static const char pass[] = "result=pass\nreason=pass\n---\n";
  static const char regr[] = "result=regression\nreason=x\n---\n";
  static const char inval[] = "result=invalid_report\nreason=x\n---\n";
  static const char unsup[] = "result=unsupported\nreason=x\n---\n";
  EXPECT(capy_benchmark_evaluation_parse(pass, sizeof(pass) - 1u, &parsed) == 1);
  EXPECT(parsed.code == CAPY_BENCHMARK_PASS);
  EXPECT(capy_benchmark_evaluation_parse(regr, sizeof(regr) - 1u, &parsed) == 1);
  EXPECT(parsed.code == CAPY_BENCHMARK_FAIL_REGRESSION);
  EXPECT(capy_benchmark_evaluation_parse(inval, sizeof(inval) - 1u, &parsed) ==
         1);
  EXPECT(parsed.code == CAPY_BENCHMARK_FAIL_INVALID_REPORT);
  EXPECT(capy_benchmark_evaluation_parse(unsup, sizeof(unsup) - 1u, &parsed) ==
         1);
  EXPECT(parsed.code == CAPY_BENCHMARK_FAIL_UNSUPPORTED);
}

static void test_evaluation_parse_rejects_malformed(void) {
  struct capy_benchmark_evaluation parsed;
  static const char unknown_result[] = "result=bogus\nreason=x\n---\n";
  static const char reordered[] = "reason=x\nresult=pass\n---\n";
  static const char empty_reason[] = "result=pass\nreason=\n---\n";
  static const char no_sentinel[] = "result=pass\nreason=x\n";
  static const char trailing[] = "result=pass\nreason=x\n---\nx";
  parsed.code = (enum capy_benchmark_result_code)7;
  /* unknown result token: parses structurally but fails the token mapping,
     and out is zeroed (PASS == 0) on failure */
  EXPECT(capy_benchmark_evaluation_parse(unknown_result,
                                         sizeof(unknown_result) - 1u,
                                         &parsed) == 0);
  EXPECT(parsed.code == CAPY_BENCHMARK_PASS);
  EXPECT(capy_benchmark_evaluation_parse(reordered, sizeof(reordered) - 1u,
                                         &parsed) == 0);
  EXPECT(capy_benchmark_evaluation_parse(empty_reason, sizeof(empty_reason) - 1u,
                                         &parsed) == 0);
  EXPECT(capy_benchmark_evaluation_parse(no_sentinel, sizeof(no_sentinel) - 1u,
                                         &parsed) == 0);
  EXPECT(capy_benchmark_evaluation_parse(trailing, sizeof(trailing) - 1u,
                                         &parsed) == 0);
}

static void test_evaluation_parse_rejects_null_text(void) {
  struct capy_benchmark_evaluation parsed;
  parsed.code = (enum capy_benchmark_result_code)7;
  EXPECT(capy_benchmark_evaluation_parse(NULL, 0u, &parsed) == 0);
  EXPECT(parsed.code == CAPY_BENCHMARK_PASS);
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
  test_replay_round_trips_through_parse();
  test_replay_parse_is_deterministic();
  test_replay_parse_accepts_max_u32();
  test_replay_parse_rejects_malformed();
  test_replay_parse_rejects_null_text();
  test_report_round_trips_through_parse();
  test_report_parse_rejects_malformed();
  test_report_parse_rejects_null_text();
  test_evaluation_round_trips_through_parse();
  test_evaluation_parse_maps_all_result_tokens();
  test_evaluation_parse_rejects_malformed();
  test_evaluation_parse_rejects_null_text();
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
