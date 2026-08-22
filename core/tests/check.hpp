#pragma once

// Decision note: zero-dependency always-live assertion harness. bare assert()
// compiles to a no-op under -DNDEBUG, which made the r1 test suite vacuous in
// the Release-configured CI job (gate finding S1). Every C++ test in this repo
// reports through ZF_CHECK so results survive any optimisation mode.

#include <cstdio>

namespace zftest {

inline int& failure_count() {
  static int count = 0;
  return count;
}

inline void report(bool condition, const char* expr, const char* file, int line) {
  if (!condition) {
    std::fprintf(stderr, "ZF_CHECK failed: %s (%s:%d)\n", expr, file, line);
    ++failure_count();
  }
}

}  // namespace zftest

#define ZF_CHECK(condition) \
  ::zftest::report(static_cast<bool>(condition), #condition, __FILE__, __LINE__)
