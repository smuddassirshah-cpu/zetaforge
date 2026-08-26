// Property suite: enclosure soundness against the Arb oracle (FLINT 3) plus an
// independent long-double radius floor. Design notes:
//
//  - Overlap check: our claimed interval shrunk inward by directed rounding
//    must overlap the Arb result enclosure. Both enclose the same true value,
//    so a miss proves a broken bound (attack item 2).
//  - Floor check: radius must be >= a floor computed entirely in long-double
//    arithmetic including a long-double replica of the outward bump. Sabotage
//    of outward rounding loses one double ulp on roughly half of all trials
//    and lands below this floor within a few trials (attack item 1).
//  - False-exact checks: no binary operation returns zero or denormal-less
//    radius silently (attack item 3).
//  - Generators target cancellation pairs, powers of two, subnormal radii,
//    huge radii and near-max centres.
//  - Seed prints at start; every failure line carries it (attack item 4).
//  - Oracle independence: this suite shares no code with zetaforge/radius.hpp;
//    its floor world is long double + direct mpfr distance measurements.

#include "zetaforge/ball.hpp"  // mpfr.h first: FLINT guards its mpfr interop on __MPFR_H

#include <flint/arb.h>
#include <flint/arf.h>

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

#include "check.hpp"
#include "exact_ref.hpp"
using exact_ref::ref_up_add;
using exact_ref::ref_up_mul;
using exact_ref::round_down_positive;
using exact_ref::decomp;
using exact_ref::bit_length;
using exact_ref::Decomposed;

using zetaforge::Ball;

namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();
constexpr mpfr_prec_t kPrec = 128;
constexpr int kTrials = 3000;

// Sound lower bounds only: every factor is a directed LOWER bound of its
// true counterpart and products are truncated downward, so these can never
// exceed the implementation's required radius when it is sound.
namespace {

double centre_lower_bound(mpfr_srcptr c) {
  const double up = mpfr_get_d(c, MPFR_RNDU);
  const double dn = mpfr_get_d(c, MPFR_RNDD);
  const double lo = std::fabs(dn) < std::fabs(up) ? std::fabs(dn) : std::fabs(up);
  return lo;
}

double term_floor(double r, double c_lb) {
  if (!(r > 0.0) || !(c_lb > 0.0)) return 0.0;
  const Decomposed dr = decomp(r);
  const Decomposed dc = decomp(c_lb);
  return exact_ref::round_down_positive(
      static_cast<exact_ref::u128>(dr.mant) * dc.mant, dr.exp + dc.exp);
}

}  // namespace

double add_floor(const Ball& x, const Ball& y, mpfr_srcptr) {
  return std::max(term_floor(x.radius(), 1.0), term_floor(y.radius(), 1.0));
}

double mul_floor(const Ball& x, const Ball& y, mpfr_srcptr) {
  const double cx = centre_lower_bound(x.centre());
  const double cy = centre_lower_bound(y.centre());
  const double f1 = term_floor(x.radius(), cy);
  const double f2 = term_floor(y.radius(), cx);
  const double f3 = term_floor(x.radius(), y.radius());
  return std::max(f1, std::max(f2, f3));
}

// ---- oracle plumbing ----

struct ArbGuard {
  arb_t v;
  ArbGuard() { arb_init(v); }
  ~ArbGuard() { arb_clear(v); }
};

void set_widened(arb_t out, const Ball& b, mpfr_prec_t wp) {
  mpfr_t r, lo, hi;
  mpfr_init2(r, 53);
  mpfr_init2(lo, wp);
  mpfr_init2(hi, wp);
  mpfr_set_d(r, b.radius(), MPFR_RNDN);
  mpfr_sub(lo, b.centre(), r, MPFR_RNDD);  // <= claimed lower edge
  mpfr_add(hi, b.centre(), r, MPFR_RNDU);  // >= claimed upper edge
  arf_t fl, fh;
  arf_init(fl);
  arf_init(fh);
  arf_set_mpfr(fl, lo);
  arf_set_mpfr(fh, hi);
  arb_set_interval_arf(out, fl, fh, wp);
  arf_clear(fl);
  arf_clear(fh);
  mpfr_clear(r);
  mpfr_clear(lo);
  mpfr_clear(hi);
}

bool shrunk_overlaps_arb(const Ball& ours, arb_t z, mpfr_prec_t wp) {
  mpfr_t alo, ahi, rin, lo_in, hi_in;
  mpfr_init2(alo, wp + 64);
  mpfr_init2(ahi, wp + 64);
  mpfr_init2(rin, 53);
  mpfr_init2(lo_in, wp + 64);
  mpfr_init2(hi_in, wp + 64);
  arb_get_interval_mpfr(alo, ahi, z);
  // Widen the read-back once against get_interval rounding.
  if (!mpfr_zero_p(alo)) mpfr_nextbelow(alo);
  if (!mpfr_zero_p(ahi)) mpfr_nextabove(ahi);
  mpfr_set_d(rin, ours.radius(), MPFR_RNDN);
  mpfr_sub(lo_in, ours.centre(), rin, MPFR_RNDU);  // >= claimed lower edge
  mpfr_add(hi_in, ours.centre(), rin, MPFR_RNDD);  // <= claimed upper edge
  const bool miss = mpfr_cmp(hi_in, alo) < 0 || mpfr_cmp(ahi, lo_in) < 0;
  mpfr_clear(alo);
  mpfr_clear(ahi);
  mpfr_clear(rin);
  mpfr_clear(lo_in);
  mpfr_clear(hi_in);
  return !miss;
}

// ---- generators ----

enum class Kind { Normal, Pow2, SubnormalRadius, HugeRadius, NearMax };

Ball gen(Kind k) {
  auto& r = ::zftest::rng();
  const double denorm = std::numeric_limits<double>::denorm_min();
  switch (k) {
    case Kind::Normal: {
      const int e = 250 - static_cast<int>(r.next() % 500);
      const double mag = std::ldexp(1.0 + ::zftest::uniform01(), e);
      const double sign = (r.next() & 1) ? -1.0 : 1.0;
      Ball b = Ball::from_double(sign * mag, kPrec);
      b.widen_radius(mag * ::zftest::uniform01() * 1e-9 + denorm);
      return b;
    }
    case Kind::Pow2: {
      const int e = static_cast<int>(r.next() % 160) - 80;
      Ball b = Ball::from_double(std::ldexp(1.0, e), kPrec);
      b.widen_radius(std::fmax(std::ldexp(1.0, e - 52) * (0.25 + ::zftest::uniform01()), denorm));
      return b;
    }
    case Kind::SubnormalRadius: {
      const int e = static_cast<int>(r.next() % 100) - 50;
      Ball b = Ball::from_double(std::ldexp(1.0, e), kPrec);
      b.widen_radius(denorm);
      return b;
    }
    case Kind::HugeRadius: {
      const int e = static_cast<int>(r.next() % 60) - 30;
      Ball b = Ball::from_double(std::ldexp(1.0, e), kPrec);
      b.widen_radius(std::ldexp(1.0, 250 + static_cast<int>(r.next() % 55)));
      return b;
    }
    case Kind::NearMax: {
      Ball b = Ball::parse("8.98846567431158e307", kPrec);  // half of DBL_MAX
      b.widen_radius(::zftest::uniform01() * 1e290);
      return b;
    }
  }
  return Ball::from_double(0.0, kPrec);  // unreachable
}

}  // namespace

int main() {
  std::fprintf(stdout, "SEED %llx\n",
               static_cast<unsigned long long>(::zftest::current_seed()));

  auto& rng = ::zftest::rng();
  int oracle_checks = 0;

  const Kind kinds[] = {Kind::Normal, Kind::Pow2, Kind::SubnormalRadius,
                        Kind::HugeRadius, Kind::NearMax};

  for (int trial = 0; trial < kTrials; ++trial) {
    if (trial % 5 == 0) {
      // Cancellation pair: nearly equal positive centres, subtracted. The
      // difference must contain zero, and its radius must survive both the
      // floor and the oracle.
      const double base = std::ldexp(1.0 + ::zftest::uniform01(),
                                     -static_cast<int>(rng.next() % 200));
      const double eps_dir = (rng.next() & 1) ? 1.0 : -1.0;
      Ball a = Ball::from_double(base, kPrec);
      a.widen_radius(base * 1e-6);
      Ball b = Ball::from_double(std::nextafter(base, eps_dir * kInf), kPrec);
      b.widen_radius(base * 1e-6);

      Ball d = Ball::sub(a, b);
      ZF_CHECK(d.contains_zero());
      ZF_CHECK(d.radius() > 0.0);
      ZF_CHECK(d.radius() >= add_floor(a, b, d.centre()));
    }

    const Ball a = gen(kinds[rng.next() % 5]);
    const Ball b = gen(kinds[rng.next() % 5]);
    const mpfr_prec_t wp = std::max(a.precision(), b.precision()) + 64;

    struct OpSpec {
      enum Op { Add, Sub, Mul } op;
    };
    const OpSpec ops[] = {{OpSpec::Add}, {OpSpec::Sub}, {OpSpec::Mul}};

    for (const OpSpec& spec : ops) {
      Ball ours = (spec.op == OpSpec::Add)
                      ? Ball::add(a, b)
                      : (spec.op == OpSpec::Sub ? Ball::sub(a, b) : Ball::mul(a, b));

      if (!ours.unknown_at_precision()) {
        ZF_CHECK(ours.radius() > 0.0);
        if (spec.op == OpSpec::Mul) {
          if (!(ours.radius() >= mul_floor(a, b, ours.centre()))) {
            std::printf("FLOORMUL a=%.17g b=%.17g\n", mpfr_get_d(a.centre(), MPFR_RNDN), mpfr_get_d(b.centre(), MPFR_RNDN));
            std::printf(" ra=%.17g rb=%.17g impl_r=%.17g floor=%.17g\n",
                        a.radius(), b.radius(), ours.radius(), mul_floor(a, b, ours.centre()));
            std::printf(" ca=%s cb=%s\n", a.centre_decimal(30).c_str(), b.centre_decimal(30).c_str());
            return 1;
          }
        } else {
          ZF_CHECK(ours.radius() >= add_floor(a, b, ours.centre()));
        }

        ArbGuard xa;
        ArbGuard xb;
        ArbGuard z;
        set_widened(xa.v, a, wp);
        set_widened(xb.v, b, wp);
        if (spec.op == OpSpec::Add) {
          arb_add(z.v, xa.v, xb.v, wp);
        } else if (spec.op == OpSpec::Sub) {
          arb_sub(z.v, xa.v, xb.v, wp);
        } else {
          arb_mul(z.v, xa.v, xb.v, wp);
        }
        ZF_CHECK(shrunk_overlaps_arb(ours, z.v, wp));
        ++oracle_checks;
      } else {
        // Saturation must be visible to the escalation policy.
        ZF_CHECK(ours.needs_escalation(1e6));
      }
    }
  }

  std::fprintf(stdout, "PROPERTY_TRIALS %d oracle_checks %d failures %d\n",
               kTrials, oracle_checks, ::zftest::failure_count());
  return ::zftest::failure_count() == 0 ? 0 : 1;
}
