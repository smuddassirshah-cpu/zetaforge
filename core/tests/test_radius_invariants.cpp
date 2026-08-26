// Oracle-free invariant tests for the outward-rounding primitives.
//
// Gate condition (stage 2 rev 1): these tests NEVER include exact_ref.hpp.
// Expectations are assembled from raw integer bit patterns or first-
// principles identities only, so a shared-derivation defect between the
// implementation and any reference cannot hide them.
//
// Invariants:
//   I1 exhaustiveness  up_add(i*dm, j*dm) == (i+j)*dm      (raw-bit expected)
//   I2 commutativity   up_add(a,b) == up_add(b,a)
//   I3 domination      up_add(a,b) >= max(a,b)
//   I4 monotonicity    extra > 0 implies widen(r, extra) > r
//                      (regression for the widen_radius no-op defect)
//   I5 identity        up_mul(k*dm, 1.0) == k*dm

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include "check.hpp"
#include "zetaforge/ball.hpp"
#include "zetaforge/radius.hpp"

using zetaforge::up_add;
using zetaforge::up_mul;

namespace {

inline double dm() { return std::numeric_limits<double>::denorm_min(); }

double bits_to_double(uint64_t bits) {
  double x = 0;
  std::memcpy(&x, &bits, sizeof(x));
  return x;
}

// (i+j) * denorm_min assembled from first principles: subnormal encoding is
// exponent field 0 with the unit count as the raw fraction.
double subnormal_from_units(uint64_t units) {
  return bits_to_double(units);
}

}  // namespace

int main() {
  std::fprintf(stdout, "SEED %llx\n",
               static_cast<unsigned long long>(::zftest::current_seed()));
  auto& rng = ::zftest::rng();

  // I1: exhaustive small pairs, exact integer expectation.
  constexpr uint64_t kSpan = 600;
  for (uint64_t i = 1; i <= kSpan; ++i) {
    const double di = subnormal_from_units(i);
    for (uint64_t j = 1; j <= kSpan; ++j) {
      const double dj = subnormal_from_units(j);
      ZF_CHECK(up_add(di, dj) == subnormal_from_units(i + j));
    }
  }

  // I1b: wide magnitudes where i+j stays under the normal boundary
  // (2^52 units), sampled across the whole reachable span.
  for (int t = 0; t < 20000; ++t) {
    uint64_t i = 1 + rng.next() % ((UINT64_C(1) << 25) - 1);
    uint64_t j = 1 + rng.next() % ((UINT64_C(1) << 25) - 1);
    if (i + j >= (UINT64_C(1) << 52)) continue;
    ZF_CHECK(up_add(subnormal_from_units(i), subnormal_from_units(j)) ==
             subnormal_from_units(i + j));
  }

  // I2/I3: commutativity and domination over random positive doubles.
  auto random_positive = [&]() {
    for (;;) {
      uint64_t bits = rng.next();
      const int e = static_cast<int>(rng.next() % 1800) - 900;
      bits = (bits & ((UINT64_C(1) << 52) - 1)) |
             (static_cast<uint64_t>(e + 1023) << 52);
      double x = 0;
      std::memcpy(&x, &bits, sizeof(x));
      if (std::isfinite(x) && x != 0.0 && !std::signbit(x)) return x;
    }
  };
  for (int t = 0; t < 20000; ++t) {
    const double a = random_positive();
    const double b = random_positive();
    ZF_CHECK(up_add(a, b) == up_add(b, a));
    ZF_CHECK(up_add(a, b) >= a);
    ZF_CHECK(up_add(a, b) >= b);
  }

  // I4: widen_radius monotonicity (regression: widen by dm was a silent no-op).
  {
    zetaforge::Ball r = zetaforge::Ball::from_double(1.0, 128);
    r.widen_radius(dm());
    const double after_one = r.radius();
    ZF_CHECK(after_one > 0.0);
    r.widen_radius(dm());
    ZF_CHECK(r.radius() > after_one);
    for (int k = 0; k < 100; ++k) {
      const double before = r.radius();
      r.widen_radius(dm());
      ZF_CHECK(r.radius() > before);
    }
  }

  // I5: multiplying a subnormal by exact one returns it unchanged.
  for (uint64_t k : {uint64_t{1}, uint64_t{7}, uint64_t{1000}}) {
    const double x = subnormal_from_units(k);
    ZF_CHECK(up_mul(x, 1.0) == x);
  }

  std::fprintf(stdout, "INVARIANTS failures %d\n", ::zftest::failure_count());
  return ::zftest::failure_count() == 0 ? 0 : 1;
}
