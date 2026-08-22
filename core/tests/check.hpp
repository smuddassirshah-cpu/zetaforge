#pragma once

// Decision note: zero-dependency always-live assertion harness. bare assert()
// compiles to a no-op under -DNDEBUG, which made the r1 test suite vacuous in
// the Release-configured CI job (gate finding S1). Every C++ test in this repo
// reports through ZF_CHECK so results survive any optimisation mode.
//
// Seed reporting (stage 2 inherited obligation): randomised property tests run
// on a deterministic splitmix64 stream. The seed is fixed by default so CI
// failures always reproduce; ZFTEST_SEED overrides it for exploration. Every
// failure line carries the active seed, and a forced failure re-run under the
// same seed must reproduce identically (stage 2 attack list, item 4).

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace zftest {

inline int& failure_count() {
  static int count = 0;
  return count;
}

inline uint64_t& current_seed() {
  static uint64_t seed = []() -> uint64_t {
    const char* env = std::getenv("ZFTEST_SEED");
    if (env != nullptr) {
      return std::strtoull(env, nullptr, 16);
    }
    return UINT64_C(0x5EEDC0DE20260822);
  }();
  return seed;
}

// Deterministic PRNG state, split apart from the recorded seed so the printed
// seed stays meaningful across runs of different length.
struct SplitMix64 {
  uint64_t state;
  explicit SplitMix64(uint64_t s) : state(s) {}
  uint64_t next() {
    uint64_t z = (state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
  }
};

inline SplitMix64& rng() {
  static SplitMix64 instance(current_seed() ^ UINT64_C(0xA5A5F00DF00D));
  return instance;
}

// Uniform double in [0, 1): derived from raw bits, portable and exact.
inline double uniform01() {
  return static_cast<double>(rng().next() >> 11) / 9007199254740992.0;
}

inline void report(bool condition, const char* expr, const char* file, int line) {
  if (!condition) {
    std::fprintf(stderr,
                 "ZF_CHECK failed: %s (%s:%d) [seed %llx]\n",
                 expr,
                 file,
                 line,
                 static_cast<unsigned long long>(current_seed()));
    ++failure_count();
  }
}

}  // namespace zftest

#define ZF_CHECK(condition) \
  ::zftest::report(static_cast<bool>(condition), #condition, __FILE__, __LINE__)
