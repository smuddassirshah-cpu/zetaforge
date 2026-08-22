#pragma once

// Canonical integer-exact outward-rounding reference for tests.
//
// Single source of truth shared by test_radius and test_ball_oracle. Every
// mantissa product is widened to unsigned __int128 EXPLICITLY at the
// multiplication site (a uint64*uint64 wraparound here once produced 36k
// phantom failures by corrupting the reference, not the implementation).
// Adjudication history lives in docs/DECISIONS.md.

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace exact_ref {

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

inline int bit_length(unsigned __int128 v) {
  int n = 0;
  while (v != 0) {
    v >>= 1;
    ++n;
  }
  return n;
}

// Smallest double >= M * 2^E (M > 0). Overflow -> inf; below denorm_min ->
// denorm_min. `sticky` folds lost low bits of an aligned sum into rounding.
inline double round_up_positive(unsigned __int128 M, int E, bool sticky_in = false) {
  if (M == 0 && !sticky_in) return 0.0;
  const int bl = bit_length(M);
  unsigned __int128 q;
  bool rem = sticky_in;
  if (bl > 53) {
    const int s = bl - 53;
    q = M >> s;
    rem = rem || ((M & (((unsigned __int128)1 << s) - 1)) != 0);
    E += s;
  } else {
    q = M << (53 - bl);
    E -= 53 - bl;
  }
  if (rem) ++q;
  if (q == ((unsigned __int128)1 << 53)) {
    q >>= 1;
    ++E;
  }
  const int fe = E + 52;
  if (fe > 1023) return kInf;
  if (fe >= -1022) {
    const uint64_t bits = (static_cast<uint64_t>(fe + 1023) << 52) |
                          static_cast<uint64_t>(q - ((unsigned __int128)1 << 52));
    double out;
    std::memcpy(&out, &bits, sizeof(out));
    return out;
  }
  // Subnormal range: ceil(M * 2^E / 2^-1074) units.
  const int sh = E + 1074;
  unsigned __int128 units;
  if (sh >= 127) {
    return kInf;
  }
  if (sh >= 0) {
    units = M << sh;
  } else {
    if (-sh > 127) {
      // strictly below half denorm_min rounds up to denorm_min anyway
      return std::numeric_limits<double>::denorm_min();
    }
    const unsigned __int128 d = (unsigned __int128)1 << (-sh);
    units = (M + d - 1) / d;
  }
  if (units >= ((unsigned __int128)1 << 52)) return kInf;
  const double out = std::ldexp(static_cast<double>(units), -1074);
  return out > 0.0 ? out : std::numeric_limits<double>::denorm_min();
}

// Largest double <= M * 2^E (M > 0), truncating sticky bits. Used only as a
// sound LOWER-bound floor for statistical suites: it can never exceed the
// true value, so an implementation below it is provably unsound.
inline double round_down_positive(unsigned __int128 M, int E) {
  if (M == 0) return 0.0;
  const int bl = bit_length(M);
  unsigned __int128 q;
  if (bl > 53) {
    const int s = bl - 53;
    q = M >> s;
    E += s;
  } else {
    q = M << (53 - bl);
    E -= 53 - bl;
  }
  const int fe = E + 52;
  if (fe > 1023) return kInf;
  if (fe >= -1022) {
    const uint64_t bits = (static_cast<uint64_t>(fe + 1023) << 52) |
                          static_cast<uint64_t>(q - ((unsigned __int128)1 << 52));
    double out;
    std::memcpy(&out, &bits, sizeof(out));
    return out;
  }
  const int sh = E + 1074;
  if (sh >= 0) {
    if (sh > 127) return kInf;
    const unsigned __int128 units = M << sh;
    if (units >= ((unsigned __int128)1 << 52)) return kInf;
    const double out = std::ldexp(static_cast<double>(units), -1074);
    return out > 0.0 ? out : std::numeric_limits<double>::denorm_min();
  }
  if (-sh > 127) return 0.0;
  const unsigned __int128 units = M >> (-sh);
  if (units == 0) return 0.0;
  if (units >= ((unsigned __int128)1 << 52)) return kInf;
  const double out = std::ldexp(static_cast<double>(units), -1074);
  return out > 0.0 ? out : std::numeric_limits<double>::denorm_min();
}

inline double compose(bool neg, unsigned __int128 q, int exp) {
  double m = static_cast<double>(q);
  return neg ? -std::ldexp(m, exp) : std::ldexp(m, exp);
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
  unsigned __int128 sum = static_cast<unsigned __int128>(hi.mant);
  bool sticky = false;
  if (shift <= 127) {
    if (shift > 0) {
      sticky = (static_cast<unsigned __int128>(lo.mant) &
                (((unsigned __int128)1 << shift) - 1)) != 0;
      sum += static_cast<unsigned __int128>(lo.mant) >> shift;
    } else {
      sum += static_cast<unsigned __int128>(lo.mant);
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
  const unsigned __int128 p =
      static_cast<unsigned __int128>(da.mant) * db.mant;  // explicit widen
  return round_up_positive(p, da.exp + db.exp);
}

}  // namespace exact_ref
