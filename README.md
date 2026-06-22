# CapyBenchmark

Version: 0.0.11

CapyBenchmark owns deterministic benchmark report validation for external CapyOS services.

## Validation

```sh
make validate
```

The release gate compiles with strict C warnings, runs benchmark contract tests, checks release metadata and verifies hardened compile flags.
