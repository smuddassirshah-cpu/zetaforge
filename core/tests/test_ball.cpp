#include <cmath>
#include <limits>
#include <stdexcept>

#include "check.hpp"
#include "zetaforge/ball.hpp"

using zetaforge::Ball;

int main() {
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

  return ::zftest::failure_count() == 0 ? 0 : 1;
}
