#pragma once

// Decision note: outward rounding primitives for the radius path. The
// invariant these implement is the project's correctness core: every result
// is greater than or equal to the exact real value of its operation on
// non-negative operands.
//
// History worth keeping visible: the first revision computed the product
// residual via std::fma(a, b, -r) and bumped on resid > 0. That is unsound:
// when the product rounds upward, the true residual can sit far below
// denorm_min (observed at ~1.7e-484), the correctly rounded fma returns 0,
// and the primitive silently returns one ulp BELOW the exact product -- an
// enclosure hole invisible to statistical tests and caught only by the
// bit-exact integer reference in core/tests/test_radius.cpp. The primitives
// therefore now compute exact integer decompositions directly; there are no
// residuals anywhere on this path.
//
// Portability: u128 (GCC/Clang), consistent with ntt.hpp and
// ffix.hpp. Soundness depends on round-to-nearest being irrelevant here --
// these paths never consult the ambient rounding mode.

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <utility>

namespace zetaforge {

// ISO C++ has no 128-bit integer type, so GCC's -Wpedantic diagnoses every
// spelling of i128 and -Werror then fails the build. __extension__
// suppresses the diagnostic at this single declaration site; uses of the
// typedef names are not re-diagnosed. Dropping -Wpedantic instead would give
// up every other portability diagnostic in order to silence one extension we
// have already decided to depend on.
__extension__ typedef unsigned __int128 u128;
__extension__ typedef __int128 i128;

namespace radius_detail {

constexpr uint64_t kMantMask = (UINT64_C(1) << 52) - 1;

// Exact decomposition for finite non-zero x: |x| = mant * 2^exp.
inline void decompose(double x, uint64_t& mant, int& exp) {
  uint64_t bits = 0;
  std::memcpy(&bits, &x, sizeof(bits));
  const uint64_t frac = bits & kMantMask;
  const int raw_exp = static_cast<int>((bits >> 52) & 0x7FF);
  if (raw_exp == 0) {
    mant = frac;
    exp = -1074;
  } else {
    mant = frac | (UINT64_C(1) << 52);
    exp = raw_exp - 1075;
  }
}

// Smallest finite non-negative double >= M * 2^E for M > 0.
// Overflow saturates to INFINITY; results below denorm_min floor to
// denorm_min (never a false-exact zero).
inline double round_up_positive(u128 M, int E,
                                bool sticky_in = false) {
  bool sticky = sticky_in;
  if (M == 0 && !sticky) {
    return 0.0;
  }
  if (M == 0) {
    return std::numeric_limits<double>::denorm_min();
  }
  int bl = 0;
  {
    u128 t = M;
    while (t != 0) {
      t >>= 1;
      ++bl;
    }
  }
  u128 q;
  if (bl > 53) {
    const int s = bl - 53;
    q = M >> s;
    sticky = sticky || ((M & (((u128)1 << s) - 1)) != 0);
    E += s;
  } else {
    q = M << (53 - bl);
    E -= 53 - bl;
  }
  if (sticky) ++q;
  if (q == ((u128)1 << 53)) {
    q >>= 1;
    ++E;
  }

  const int fe = E + 52;  // binade exponent: value = (q/2^52) * 2^fe
  if (fe > 1023) {
    return INFINITY;
  }
  if (fe >= -1022) {
    const uint64_t bits =
        (static_cast<uint64_t>(fe + 1023) << 52) |
        static_cast<uint64_t>(q - ((u128)1 << 52));
    double out = 0;
    std::memcpy(&out, &bits, sizeof(out));
    return out;
  }
  // Subnormal range (fe < -1022 implies E < -1074 here): value = q * 2^E
  // with (q, E) the coherent post-normalisation pair. Units of denorm_min =
  // ceil(q / 2^sh) with sh = -(E + 1074) >= 0. Gate finding: this branch
  // previously re-read pre-normalisation M against the mutated E, returning
  // half-truths across the entire subnormal range.
  const int sh = -(E + 1074);
  u128 units;
  if (sh > 127) {
    // q < 2^53 <= 2^sh: the value is nonzero but rounds up to one unit.
    units = 1;
  } else {
    const u128 d = (u128)1 << sh;
    units = (q + d - 1) / d;
  }
  if (units >= ((u128)1 << 52)) {
    return INFINITY;  // cannot occur from fe < -1022; defensive escalate
  }
  const double out = std::ldexp(static_cast<double>(units), -1074);
  return out > 0.0 ? out : std::numeric_limits<double>::denorm_min();
}

}  // namespace radius_detail

// Smallest double strictly greater than or equal to the exact sum a+b,
// for non-negative finite a, b.
inline double up_add(double a, double b) noexcept {
  if (!(a >= 0.0 && b >= 0.0) || !std::isfinite(a) || !std::isfinite(b)) {
    return std::numeric_limits<double>::infinity();  // precondition violation: refuse enclosure (never 0: false-exact)
  }
  if (a == 0.0) return b;
  if (b == 0.0) return a;
  uint64_t ma, mb;
  int ea, eb;
  radius_detail::decompose(a, ma, ea);
  radius_detail::decompose(b, mb, eb);
  if (ea < eb) {
    std::swap(ma, mb);
    std::swap(ea, eb);
  }
  const int shift = ea - eb;
  u128 sum = static_cast<u128>(ma);
  bool sticky = false;
  if (shift <= 127) {
    if (shift > 0) {
      // shift may exceed 64: mb must be widened before shifting or masking
      const u128 mbw = static_cast<u128>(mb);
      sticky = (mbw & (((u128)1 << shift) - 1)) != 0;
      sum += mbw >> shift;
    } else {
      sum += mb;
    }
  } else {
    sticky = mb != 0;
  }
  // sum is exact at scale ea (the larger exponent); sticky carries the loss.
  return radius_detail::round_up_positive(sum, ea, sticky);
}

// Smallest double >= the exact product a*b, for non-negative finite a, b.
inline double up_mul(double a, double b) noexcept {
  if (!(a >= 0.0 && b >= 0.0) || !std::isfinite(a) || !std::isfinite(b)) {
    return std::numeric_limits<double>::infinity();  // precondition violation: refuse enclosure (never 0: false-exact)
  }
  if (a == 0.0 || b == 0.0) {
    return 0.0;
  }
  uint64_t ma, mb;
  int ea, eb;
  radius_detail::decompose(a, ma, ea);
  radius_detail::decompose(b, mb, eb);
  return radius_detail::round_up_positive(
      static_cast<u128>(ma) * mb, ea + eb);
}

// One ulp upward; maps zero to the smallest positive subnormal so an inexact
// radius can never collapse to a false-exact zero.
inline double inflate(double x) noexcept {
  if (!(x < INFINITY)) {
    return x;
  }
  if (x <= 0.0) {
    return std::numeric_limits<double>::denorm_min();
  }
  return std::nextafter(x, INFINITY);
}

// Conservative bound on |exact - c| for a real number whose nearest-double
// representation is c: half the wider ulp span at c. Both distances are
// Sterbenz-exact subtractions, so the bound itself is trustworthy.
inline double half_ulp_bound(double c) noexcept {
  if (!std::isfinite(c)) {
    return INFINITY;
  }
  if (c == 0.0) {
    return std::numeric_limits<double>::denorm_min();
  }
  const double up = std::nextafter(c, INFINITY);
  const double dn = std::nextafter(c, -INFINITY);
  const double half = std::fmax(up - c, c - dn) * 0.5;
  // Subnormal c: the wider span IS denorm_min, and halving it lands exactly on
  // 2^-1075, which ties-to-even rounds to zero. Returning that would assert
  // certainty about an inexact value, the false-exact failure the stage 2
  // ledger pre-registered but only ever probed on up_mul (review finding B6).
  // denorm_min is the smallest sound answer here and is outward.
  return half > 0.0 ? half : std::numeric_limits<double>::denorm_min();
}

}  // namespace zetaforge
