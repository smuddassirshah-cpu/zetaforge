// Bit-exact verification of the outward-rounding primitives against the
// canonical integer reference in exact_ref.hpp.
//
// History: two earlier reference bugs produced tens of thousands of phantom
// failures (a missing exponent compensation and a uint64 mantissa-product
// wraparound), and one real implementation bug was found in the same period
// (std::fma residual flushing below denorm_min). The suite now shares ONE
// reference implementation with explicit widening at every product site.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

#include "check.hpp"
#include "exact_ref.hpp"
#include "zetaforge/ball.hpp"
#include "zetaforge/radius.hpp"

using zetaforge::half_ulp_bound;
using zetaforge::up_add;
using zetaforge::up_mul;
using exact_ref::ref_up_add;
using exact_ref::ref_up_mul;
using exact_ref::round_down_positive;
using exact_ref::round_up_positive;
using exact_ref::bit_length;
using exact_ref::decomp;
using exact_ref::Decomposed;
using exact_ref::compose;

namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();

double random_normal_finite() {
  auto& r = ::zftest::rng();
  for (;;) {
    uint64_t bits = r.next();
    const int e = static_cast<int>(r.next() % 1800) - 900;
    bits = (bits & ((UINT64_C(1) << 52) - 1)) |
           (static_cast<uint64_t>(e + 1023) << 52);
    double x = 0;
    std::memcpy(&x, &bits, sizeof(x));
    if (std::isfinite(x) && x != 0.0 && !std::signbit(x)) return x;
  }
}

}  // namespace

int main() {
  std::fprintf(stdout, "SEED %llx\n",
               static_cast<unsigned long long>(::zftest::current_seed()));
  auto& rng = ::zftest::rng();

  std::vector<double> edges = {
      0.0,
      std::numeric_limits<double>::denorm_min(),
      5.0 * std::numeric_limits<double>::denorm_min(),
      std::numeric_limits<double>::min(),
      1.0,
      std::nextafter(1.0, kInf),
      std::nextafter(1.0, 0.0),
      2.0,
      0.5,
      1e300,
      1e-300,
      std::numeric_limits<double>::max(),
      std::numeric_limits<double>::max() / 2,
      std::nextafter(std::numeric_limits<double>::max(), 0.0),
  };
  for (int i = 0; i < 200; ++i) edges.push_back(random_normal_finite());

  // up_mul: edge cross-product plus randomised trials.
  for (double a : edges) {
    for (double b : edges) {
      if (a == 0.0 || b == 0.0 || !std::isfinite(a) || !std::isfinite(b)) continue;
      const Decomposed da = decomp(a);
      const Decomposed db = decomp(b);
      const zetaforge::u128 p =
          static_cast<zetaforge::u128>(da.mant) * db.mant;
      const int pe = da.exp + db.exp;
      const int top_exp = pe + bit_length(p) - 1;
      if (top_exp < -1021 || top_exp > 971) continue;
      ZF_CHECK(up_mul(a, b) == round_up_positive(p, pe));
    }
  }
  for (int t = 0; t < 40000; ++t) {
    const double a = random_normal_finite();
    const double b = random_normal_finite();
    ZF_CHECK(up_mul(a, b) == ref_up_mul(a, b));
  }

  // up_add: aligned-gap corpus plus randomised trials with bounded gaps.
  for (double a : edges) {
    for (double b : edges) {
      if (a == 0.0 || b == 0.0 || !std::isfinite(a) || !std::isfinite(b)) continue;
      const Decomposed da = decomp(a);
      const Decomposed db = decomp(b);
      const int gap = std::abs(da.exp - db.exp);
      if (gap > 100) continue;
      ZF_CHECK(up_add(a, b) == ref_up_add(a, b));
    }
  }
  for (int t = 0; t < 40000; ++t) {
    double a = random_normal_finite();
    double b = random_normal_finite();
    Decomposed da = decomp(a);
    Decomposed db = decomp(b);
    if (std::abs(da.exp - db.exp) > 90) {
      db.exp = da.exp - static_cast<int>(rng.next() % 90);
      b = compose(false, db.mant, db.exp);
    }
    ZF_CHECK(up_add(a, b) == ref_up_add(a, b));
  }

  // half_ulp_bound closed form: normals satisfy hulp == 2^(fe-53) except the
  // predecessor binade edge below a power of two, which is half that again.
  for (int e = -1000; e <= 1000; ++e) {
    const double p2 = std::ldexp(1.0, e);
    if (!std::isfinite(p2) || p2 == 0.0) continue;
    ZF_CHECK(half_ulp_bound(p2) == std::ldexp(1.0, e - 53));
    const double above = std::nextafter(p2, kInf);
    if (std::isfinite(above) && std::fabs(above) > std::fabs(p2)) {
      ZF_CHECK(half_ulp_bound(above) == std::ldexp(1.0, e - 53));
    }
    const double below = std::nextafter(p2, 0.0);
    if (below != 0.0 && std::isfinite(below)) {
      ZF_CHECK(half_ulp_bound(below) == std::ldexp(1.0, e - 54));
    }
  }
  for (int t = 0; t < 20000; ++t) {
    const double c = random_normal_finite();
    const Decomposed dc = decomp(c);
    ZF_CHECK(half_ulp_bound(c) == std::ldexp(1.0, dc.exp + 52 - 53));
  }

  // Targeted saturation and underflow semantics.
  ZF_CHECK(up_mul(std::numeric_limits<double>::max(), 2.0) == kInf);
  ZF_CHECK(up_add(std::numeric_limits<double>::max(),
                  std::numeric_limits<double>::max()) == kInf);
  ZF_CHECK(up_mul(std::numeric_limits<double>::denorm_min(),
                  std::numeric_limits<double>::denorm_min()) ==
           std::numeric_limits<double>::denorm_min());
  ZF_CHECK(up_mul(0.0, 7.0) == 0.0);
  ZF_CHECK(up_add(0.0, 0.0) == 0.0);

  // ---- parse radius against an independently derived reference (B4) ------
  // Production derives the bound from exponent arithmetic (2^(e-p-1)). This
  // reference derives it from mpfr_nextabove: take the successor of the stored
  // centre at its own precision, difference it, halve it. Different machinery,
  // so a single-sided slip in either shows up as disagreement rather than
  // being mirrored. Per DECISIONS.md, independence means independently
  // DERIVED, not independently filed.
  {
    const char* literals[] = {"0.5", "1.41421356237309504880", "6.62607015e-34",
                              "1e-320", "12345.6789", "1e-40"};
    for (const char* lit : literals) {
      for (const mpfr_prec_t pr : {mpfr_prec_t(64), mpfr_prec_t(128),
                                   mpfr_prec_t(256)}) {
        const zetaforge::Ball b = zetaforge::Ball::parse(lit, pr);
        if (!(b.radius() < kInf)) {
          continue;  // unknown-at-precision policy, checked in test_ball
        }
        mpfr_t nxt, span;
        mpfr_init2(nxt, pr);
        mpfr_init2(span, 256);
        mpfr_set(nxt, b.centre(), MPFR_RNDN);
        mpfr_nextabove(nxt);
        mpfr_sub(span, nxt, b.centre(), MPFR_RNDN);  // exactly one ulp at pr
        mpfr_abs(span, span, MPFR_RNDN);
        mpfr_div_2ui(span, span, 1, MPFR_RNDN);      // exactly half an ulp
        mpfr_t claimed;
        mpfr_init2(claimed, 256);
        mpfr_set_d(claimed, b.radius(), MPFR_RNDN);
        // Soundness always: the claimed bound must dominate the true half-ulp.
        ZF_CHECK(mpfr_cmp(claimed, span) >= 0);
        // Minimality where the half-ulp is representable as a double. Below
        // denorm_min the production policy deliberately clamps upward (it must
        // never report a false-exact zero), so equality is required only in
        // the representable regime; that is exactly where a one-sided slip in
        // the exponent arithmetic would hide.
        mpfr_t dmin;
        mpfr_init2(dmin, 64);
        mpfr_set_d(dmin, std::numeric_limits<double>::denorm_min(), MPFR_RNDN);
        if (mpfr_cmp(span, dmin) >= 0) {
          ZF_CHECK(mpfr_cmp(claimed, span) == 0);
        }
        mpfr_clear(dmin);
        mpfr_clear(claimed);
        mpfr_clear(nxt);
        mpfr_clear(span);
      }
    }
  }

  std::fprintf(stdout, "RADIUS_EXACT failures %d\n", ::zftest::failure_count());
  return ::zftest::failure_count() == 0 ? 0 : 1;
}
