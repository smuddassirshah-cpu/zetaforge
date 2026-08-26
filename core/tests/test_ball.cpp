#include <cstdio>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "check.hpp"
#include "zetaforge/ball.hpp"
#include "zetaforge/radius.hpp"

using zetaforge::Ball;

int main() {
  // This suite draws nothing from the seed stream, but every ZF_CHECK failure
  // line carries the active seed, so printing it keeps the record uniform
  // across the suite and makes any run self-identifying (stage 2 inherited
  // obligation, the half rev 5 carried forward).
  std::fprintf(stdout, "SEED %llx\n",
               static_cast<unsigned long long>(::zftest::current_seed()));
  // Exact scalars carry zero radius; binary ops never claim exactness.
  Ball half = Ball::from_double(0.5, 128);
  ZF_CHECK(half.radius() == 0.0);
  ZF_CHECK(!half.contains_zero());

  Ball zero = Ball::from_double(0.0, 128);
  ZF_CHECK(zero.contains_zero());

  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();

  Ball sum = Ball::add(Ball::from_double(0.5, 128), Ball::from_double(0.25, 128));
  ZF_CHECK(sum.radius() > 0.0);
  ZF_CHECK(std::fabs(mpfr_get_d(sum.centre(), MPFR_RNDN) - 0.75) < 1e-18);

  Ball diff = Ball::sub(half, Ball::from_double(0.5, 128));
  ZF_CHECK(diff.contains_zero());
  ZF_CHECK(diff.radius() > 0.0);

  Ball m = Ball::mul(Ball::from_double(4.0, 128), Ball::from_double(3.0, 128));
  ZF_CHECK(std::fabs(mpfr_get_d(m.centre(), MPFR_RNDN) - 12.0) < 1e-15);

  Ball s3 = Ball::scale(half, 3);
  ZF_CHECK(std::fabs(mpfr_get_d(s3.centre(), MPFR_RNDN) - 1.5) < 1e-18);

  // Parse carries representation doubt: radius strictly positive.
  Ball p = Ball::parse("0.1", 128);
  ZF_CHECK(p.radius() >= std::numeric_limits<double>::denorm_min());

  // Escalation policy: relative width thresholds.
  ZF_CHECK(!sum.needs_escalation(1e6));
  ZF_CHECK(sum.needs_escalation(1e-30));

  bool threw = false;
  try {
    auto b = Ball::from_double(nan, 128);
    (void)b;
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  ZF_CHECK(threw);

  threw = false;
  try {
    auto b = Ball::from_double(inf, 128);
    (void)b;
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  ZF_CHECK(threw);

  threw = false;
  try {
    auto b = Ball::parse("not-a-number", 128);
    (void)b;
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  ZF_CHECK(threw);

  threw = false;
  try {
    s3.widen_radius(-1.0);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  ZF_CHECK(threw);

  threw = false;
  try {
    s3.widen_radius(inf);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  ZF_CHECK(threw);

  threw = false;
  try {
    Ball lowprec(32);
    (void)lowprec;
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  ZF_CHECK(threw);

  // widen_radius contract: strictly increases the radius for positive extra
  // (regression for the stage 2 gate defect where subnormal extras were no-ops).
  Ball w = Ball::from_double(1.0, 128);
  const double denorm = std::numeric_limits<double>::denorm_min();
  w.widen_radius(denorm);
  const double r1 = w.radius();
  ZF_CHECK(r1 > 0.0);
  w.widen_radius(denorm);
  ZF_CHECK(w.radius() > r1);

  // ---- parse radius soundness (review findings B4, B6) -------------------
  // Both cases were enclosure violations in an approved stage 2 API: the
  // radius was derived from the DOUBLE image of the mpfr centre, so it
  // collapsed to a false-exact zero below the subnormal threshold and to
  // denorm_min above DBL_MAX.
  {
    // Truth is computed at a precision far above the ball's own, so the
    // reference is not the quantity under test.
    mpfr_t truth, diff, rad;
    mpfr_init2(truth, 512);
    mpfr_init2(diff, 512);
    mpfr_init2(rad, 512);

    // Subnormal-range magnitude: must not claim exactness, must enclose.
    {
      const Ball b = Ball::parse("1e-320", 128);
      ZF_CHECK(b.radius() > 0.0);
      mpfr_set_str(truth, "1e-320", 10, MPFR_RNDN);
      mpfr_sub(diff, b.centre(), truth, MPFR_RNDN);
      mpfr_abs(diff, diff, MPFR_RNDN);
      mpfr_set_d(rad, b.radius(), MPFR_RNDN);
      ZF_CHECK(mpfr_cmp(diff, rad) <= 0);
    }

    // Beyond double range: the ball must not claim a finite radius that sits
    // below the true representation error.
    {
      const Ball b = Ball::parse("1e400", 128);
      mpfr_set_str(truth, "1e400", 10, MPFR_RNDN);
      mpfr_sub(diff, b.centre(), truth, MPFR_RNDN);
      mpfr_abs(diff, diff, MPFR_RNDN);
      if (b.radius() < std::numeric_limits<double>::infinity()) {
        mpfr_set_d(rad, b.radius(), MPFR_RNDN);
        ZF_CHECK(mpfr_cmp(diff, rad) <= 0);
      } else {
        ZF_CHECK(b.unknown_at_precision());
      }
    }

    // A value representable exactly at the working precision still carries a
    // sound (non-negative) bound, and enclosure holds.
    {
      const Ball b = Ball::parse("0.5", 128);
      ZF_CHECK(b.radius() >= 0.0);
      mpfr_set_str(truth, "0.5", 10, MPFR_RNDN);
      mpfr_sub(diff, b.centre(), truth, MPFR_RNDN);
      mpfr_abs(diff, diff, MPFR_RNDN);
      mpfr_set_d(rad, b.radius(), MPFR_RNDN);
      ZF_CHECK(mpfr_cmp(diff, rad) <= 0);
    }

    mpfr_clear(truth);
    mpfr_clear(diff);
    mpfr_clear(rad);
  }

  // half_ulp_bound must never return zero for an inexact subnormal (B6).
  {
    const double dmin = std::numeric_limits<double>::denorm_min();
    ZF_CHECK(zetaforge::half_ulp_bound(dmin) > 0.0);
    ZF_CHECK(zetaforge::half_ulp_bound(dmin * 3.0) > 0.0);
    ZF_CHECK(zetaforge::half_ulp_bound(std::ldexp(1.0, -1073)) > 0.0);
  }

  return ::zftest::failure_count() == 0 ? 0 : 1;
}
