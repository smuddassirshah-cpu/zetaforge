#pragma once

// Canonical test-side reference for outward/truncated double rounding of
// exact dyadic values.
//
// Independence statement (stage 2 gate rev 1): this file must not share a
// DERIVATION with zetaforge/radius.hpp. radius.hpp constructs its answer by
// mantissa normalisation (mutate M into q against a mutated E, assemble).
// This reference instead derives everything from two first-principles facts
// that require no normalisation step and therefore cannot reproduce a
// normalisation bug:
//
//   1. Bit-length identity: for positive dyadic v = M * 2^E with
//      bl := bit_length(M), the containing binade exponent of v is exactly
//      fe = E + bl - 1. No mutation, no scaling: direct consequence of
//      2^(bl-1) <= M < 2^bl.
//   2. Subnormal domain in raw units of 2^-1074: results below the normal
//      range are computed as pure integer unit counts (ceiling or floor of
//      M * 2^(E + 1074)), never through any (mantissa, exponent) pair.
//
// The single final conversion is one ceiling (or floor) division once the
// quantisation exponent is known. A bug in radius.hpp's normalisation state
// machine cannot be mirrored here because no such machine exists on this
// side.

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace exact_ref {

// Declared here rather than taken from zetaforge/radius.hpp on purpose. The
// independence statement above is the reason this file exists, and it must not
// acquire a compile-time dependency on the implementation it checks, not even
// for a typedef. __extension__ silences GCC's -Wpedantic diagnostic at the
// declaration site; see radius.hpp for the same reasoning on the production
// side.
__extension__ typedef unsigned __int128 u128;

constexpr double kInf = std::numeric_limits<double>::infinity();

struct Decomposed {
  bool neg;
  uint64_t mant;
  int exp;
};

inline Decomposed decomp(double x) {
  Decomposed d;
  d.neg = std::signbit(x);
  const double ax = std::fabs(x);
  if (ax == 0.0) {
    d.mant = 0;
    d.exp = 0;
    return d;
  }
  uint64_t bits = 0;
  std::memcpy(&bits, &ax, sizeof(bits));
  const uint64_t frac = bits & ((UINT64_C(1) << 52) - 1);
  const int raw_exp = static_cast<int>((bits >> 52) & 0x7FF);
  if (raw_exp == 0) {
    d.mant = frac;
    d.exp = -1074;
  } else {
    d.mant = frac | (UINT64_C(1) << 52);
    d.exp = raw_exp - 1075;
  }
  return d;
}

inline int bit_length(u128 v) {
  int n = 0;
  while (v != 0) {
    v >>= 1;
    ++n;
  }
  return n;
}

inline double compose(bool neg, u128 q, int exp) {
  double m = static_cast<double>(q);
  return neg ? -std::ldexp(m, exp) : std::ldexp(m, exp);
}

namespace ref_detail {

inline double assemble_normal(uint64_t mant_bits, int fe) {
  const uint64_t bits = (static_cast<uint64_t>(fe + 1023) << 52) | mant_bits;
  double out = 0;
  std::memcpy(&out, &bits, sizeof(out));
  return out;
}

}  // namespace ref_detail

// Smallest double >= M * 2^E for M > 0. Sticky folds lost low bits of an
// aligned sum into the ceiling decision; pass false for bare products.
inline double round_up_positive(u128 M, int E, bool sticky_in = false) {
  const int bl = bit_length(M);
  const int fe = E + bl - 1;          // fact 1: exact containing binade
  if (fe > 1023) return kInf;

  // Quantised grid value: v / ulp = M * 2^(E - (fe - 52)), ulp = 2^(fe-52)
  const int d = fe - 52 - E;          // right-shift applied to M, >= 0 here
  u128 q;
  bool sticky = sticky_in;
  if (d <= 0) {
    // v is an exact multiple of ulp already (M fits the 53-bit grid).
    q = M << (-d);
  } else {
    q = M >> d;
    sticky = sticky || ((M & (((u128)1 << d) - 1)) != 0);
  }
  if (sticky) ++q;

  if (fe >= -1022) {
    if (q == ((u128)1 << 53)) {
      return ref_detail::assemble_normal(0, fe + 1);  // carried into next binade
    }
    return ref_detail::assemble_normal(static_cast<uint64_t>(q - ((u128)1 << 52)), fe);
  }

  // Subnormal domain: pure integer units of 2^-1074 (fact 2).
  // v = q * 2^(fe-52); units = ceil(v / 2^-1074) = ceil(q * 2^(fe+1022)).
  // fe <= -1023 here so the shift exponent is negative: divide, and any
  // nonzero remainder rounds up to one unit.
  const int sh = -(fe + 1022);
  u128 units;
  if (sh > 127) {
    units = 1;
  } else {
    const u128 dd = (u128)1 << sh;
    units = (q + dd - 1) / dd;
  }
  const double out = std::ldexp(static_cast<double>(units), -1074);
  return out > 0.0 ? out : std::numeric_limits<double>::denorm_min();
}

// Largest double <= M * 2^E for M > 0 (truncating). Same independent
// derivation with a floor division and no sticky bump.
inline double round_down_positive(u128 M, int E) {
  const int bl = bit_length(M);
  const int fe = E + bl - 1;
  if (fe > 1023) return kInf;

  const int d = fe - 52 - E;
  u128 q;
  if (d <= 0) {
    q = M << (-d);
  } else {
    q = M >> d;
  }

  if (fe >= -1022) {
    if (q >= ((u128)1 << 53)) {
      return ref_detail::assemble_normal((1ULL << 52) - 1, fe + 1);  // truncated carry edge
    }
    return ref_detail::assemble_normal(static_cast<uint64_t>(q - ((u128)1 << 52)), fe);
  }

  const int sh = -(fe + 1022);
  u128 units;
  if (sh > 127) {
    return 0.0;  // entire value rounds below the smallest subnormal step
  }
  const u128 dd = (u128)1 << sh;
  units = q / dd;
  if (units == 0) return 0.0;
  const double out = std::ldexp(static_cast<double>(units), -1074);
  return out > 0.0 ? out : std::numeric_limits<double>::denorm_min();
}

// Smallest double >= exact a+b for positive finite doubles.
inline double ref_up_add(double a, double b) {
  Decomposed hi = decomp(a);
  Decomposed lo = decomp(b);
  if (hi.exp < lo.exp) {
    Decomposed t = hi;
    hi = lo;
    lo = t;
  }
  const int shift = hi.exp - lo.exp;
  u128 sum = static_cast<u128>(hi.mant);
  bool sticky = false;
  if (shift <= 127) {
    if (shift > 0) {
      const u128 mbw = static_cast<u128>(lo.mant);
      sticky = (mbw & (((u128)1 << shift) - 1)) != 0;
      sum += mbw >> shift;
    } else {
      sum += static_cast<u128>(lo.mant);
    }
  } else {
    sticky = lo.mant != 0;
  }
  return round_up_positive(sum, hi.exp, sticky);
}

// Smallest double >= exact a*b for positive finite doubles.
inline double ref_up_mul(double a, double b) {
  const Decomposed da = decomp(a);
  const Decomposed db = decomp(b);
  const u128 p =
      static_cast<u128>(da.mant) * db.mant;  // explicit widen
  return round_up_positive(p, da.exp + db.exp);
}

}  // namespace exact_ref
