# Security Policy

CapyBenchmark 0.0.1 is an early service release. Report security issues privately to the repository owner before opening public issues.

## Release gate

- `make validate` must pass before release tags.
- Benchmark reports must fail closed when required fields or metrics are missing.
- Regression thresholds must be explicit for release comparisons.
