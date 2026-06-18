#ifndef CAPY_BENCHMARK_H
#define CAPY_BENCHMARK_H

#include <stddef.h>
#include <stdint.h>

#define CAPY_BENCHMARK_NAME_MAX 64u
#define CAPY_BENCHMARK_VERSION_MAX 32u
#define CAPY_BENCHMARK_PLATFORM_MAX 64u
#define CAPY_BENCHMARK_REASON_MAX 128u
#define CAPY_BENCHMARK_SERIAL_MAX 1024u

enum capy_benchmark_result_code {
  CAPY_BENCHMARK_PASS = 0,
  CAPY_BENCHMARK_FAIL_REGRESSION,
  CAPY_BENCHMARK_FAIL_INVALID_REPORT,
  CAPY_BENCHMARK_FAIL_UNSUPPORTED
};

struct capy_benchmark_metrics {
  uint32_t average_fps_milli;
  uint32_t p95_frame_time_us;
  uint32_t p99_frame_time_us;
  uint32_t input_latency_us;
  uint32_t cpu_usage_milli;
  uint32_t memory_peak_kib;
  uint32_t dropped_events;
  uint32_t state_checksum;
};

struct capy_benchmark_report {
  char name[CAPY_BENCHMARK_NAME_MAX];
  char benchmark_version[CAPY_BENCHMARK_VERSION_MAX];
  char runner_version[CAPY_BENCHMARK_VERSION_MAX];
  char platform[CAPY_BENCHMARK_PLATFORM_MAX];
  uint32_t replay_id;
  uint32_t seed;
  struct capy_benchmark_metrics metrics;
};

struct capy_benchmark_thresholds {
  uint32_t min_average_fps_milli;
  uint32_t max_p95_frame_time_us;
  uint32_t max_p99_frame_time_us;
  uint32_t max_input_latency_us;
  uint32_t max_cpu_usage_milli;
  uint32_t max_memory_peak_kib;
  uint32_t max_dropped_events;
  uint32_t expected_state_checksum;
  uint32_t require_state_checksum;
};

struct capy_benchmark_evaluation {
  enum capy_benchmark_result_code code;
  char reason[CAPY_BENCHMARK_REASON_MAX];
};

struct capy_benchmark_replay {
  uint32_t replay_id;
  uint32_t seed;
  uint32_t frame_budget;
};

void capy_benchmark_report_init(struct capy_benchmark_report *report,
                                const char *name,
                                const char *benchmark_version,
                                const char *runner_version,
                                const char *platform);
int capy_benchmark_report_valid(const struct capy_benchmark_report *report);
void capy_benchmark_evaluate(const struct capy_benchmark_report *report,
                             const struct capy_benchmark_thresholds *thresholds,
                             struct capy_benchmark_evaluation *out);
int capy_benchmark_report_serialize(const struct capy_benchmark_report *report,
                                    char *out, size_t out_size);
int capy_benchmark_evaluation_serialize(
    const struct capy_benchmark_evaluation *eval, char *out, size_t out_size);
int capy_benchmark_replay_valid(const struct capy_benchmark_replay *replay);
int capy_benchmark_replay_serialize(const struct capy_benchmark_replay *replay,
                                    char *out, size_t out_size);
int capy_benchmark_replay_parse(const char *text, size_t len,
                                struct capy_benchmark_replay *out);
int capy_benchmark_report_parse(const char *text, size_t len,
                                struct capy_benchmark_report *out);
int capy_benchmark_evaluation_parse(const char *text, size_t len,
                                    struct capy_benchmark_evaluation *out);

#endif
