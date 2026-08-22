// NTT convolution benchmark. Single-threaded by design; conditions recorded
// in docs/benchmarks/ntt-bench.md alongside the fitted slope.
//
// Usage: bench_ntt <n_min> <n_max>   (powers of two)

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "zetaforge/ntt.hpp"

namespace {

uint64_t g_rng = 0x9E3779B97F4A7C15ULL;

uint64_t next_rng() {
  uint64_t z = (g_rng += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

}  // namespace

int main(int argc, char** argv) {
  const size_t n_min = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 1024;
  const size_t n_max = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 1ULL << 20;

  std::printf("n,median_ns\n");
  for (size_t n = n_min; n <= n_max; n <<= 1) {
    std::vector<uint64_t> a(n), b(n);
    for (size_t i = 0; i < n; ++i) {
      a[i] = next_rng() % 998244353ULL;
      b[i] = next_rng() % 998244353ULL;
    }
    // warm-up + repeats, median of 7
    std::vector<double> samples;
    for (int rep = 0; rep < 9; ++rep) {
      const auto t0 = std::chrono::steady_clock::now();
      volatile size_t sink = zetaforge::convolve_linear(a, b).size();
      (void)sink;
      const auto t1 = std::chrono::steady_clock::now();
      if (rep >= 2) {
        samples.push_back(
            std::chrono::duration<double, std::nano>(t1 - t0).count());
      }
    }
    std::sort(samples.begin(), samples.end());
    const double median = samples[samples.size() / 2];
    std::printf("%zu,%.1f\n", n, median);
  }
  return 0;
}
