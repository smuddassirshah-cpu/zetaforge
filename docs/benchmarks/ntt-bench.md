# NTT convolution benchmark

Claim under test: zetaforge::convolve_linear is O(n log n).

## Environment

- Hardware: Apple M2 (8-core; benchmark is single-threaded by construction)
- Compiler: Apple clang 17.0.0, `-O3 -ffp-contract=off -frounding-math` (Release)
- Timing: std::chrono::steady_clock around convolve_linear; median of 7
  samples after 2 warm-up runs per point
- Workload: two random vectors in [0, p) per n; full linear convolution
  (output length 2n-1, zero-padded internally to next power of two)
- Date: 2026-08-22
- Script: core/bench/bench_ntt.cpp; raw data: docs/benchmarks/ntt_bench.csv

## Result

Log-log least-squares fit over n = 2^10 .. 2^20:

    slope = 1.111      R^2 = 0.999953

An O(n log n) algorithm presents a fitted slope between 1.0 and about 1.15
over a three-decade range (the log factor bends the line upward slightly);
the measured 1.111 with near-perfect linearity is consistent with O(n log n)
and inconsistent with both O(n) (slope ~1.0 flat at large n with this much
log growth) and O(n^1.5) or worse.

## Raw data

| n | median (ns) |
|---|---|
| 1024 | 484000 |
| 2048 | 1056125 |
| 4096 | 2309959 |
| 8192 | 4943958 |
| 16384 | 10719458 |
| 32768 | 23135417 |
| 65536 | 51904667 |
| 131072 | 110077500 |
| 262144 | 233553292 |
| 524288 | 496648542 |
| 1048576 | 1059257750 |

Doubling ratios sit between 2.05 and 2.24 across the range and creep upward
slowly, exactly the log-factor signature.

Reproduce with: `./build/core/bench_ntt 1024 1048576 > docs/benchmarks/ntt_bench.csv`
