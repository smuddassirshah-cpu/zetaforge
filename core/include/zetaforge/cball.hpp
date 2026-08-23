#pragma once

// Complex interval (ball) arithmetic over an mpfr pair with double
// componentwise radii.
//
// Soundness contract: for every point inside the input boxes, the true
// product/sum lies within the returned component radii.
//
// Design notes:
// - Construction takes precision; destructor frees mpfr state.
// - Copy/move implement Rule of Five properly (no pointer aliasing).
// - Centre magnitudes use MPFR_RNDU for conservative overestimates.
// - Radius includes centre-rounding term proportional to product magnitude
//   at working precision (so two zero-radius inputs still produce nonzero
//   representation-error radius).
// - mul_real uses outward-rounded internal products.

#include <cmath>
#include <limits>
#include <utility>
#include <mpfr.h>

namespace zetaforge {

namespace cb {

inline double up_add(double a, double b) noexcept {
  const double s = a + b;
  if (!std::isfinite(s)) return s;
  const double bv = s - a;
  const double err = (a - (s - bv)) + (b - bv);
  return err > 0.0 ? std::nextafter(s, INFINITY) : s;
}

inline double fabs_upper(const mpfr_t x) noexcept {
  return std::fabs(mpfr_get_d(x, MPFR_RNDU));
}

}  // namespace cb

struct CBall {
  mpfr_t re, im;
  double rr = 0.0, ri = 0.0;

  explicit CBall(mpfr_prec_t p) {
    mpfr_init2(re, p);
    mpfr_init2(im, p);
    mpfr_set_zero(re, 0);
    mpfr_set_zero(im, 0);
  }

  ~CBall() {
    mpfr_clear(re);
    mpfr_clear(im);
  }

  CBall(const CBall& o)
    : rr(o.rr), ri(o.ri) {
    mpfr_init2(re, mpfr_get_prec(o.re));
    mpfr_init2(im, mpfr_get_prec(o.im));
    mpfr_set(re, o.re, MPFR_RNDN);
    mpfr_set(im, o.im, MPFR_RNDN);
  }

  CBall& operator=(const CBall& o) {
    if (this != &o) {
      mpfr_set_prec(re, mpfr_get_prec(o.re));
      mpfr_set_prec(im, mpfr_get_prec(o.im));
      mpfr_set(re, o.re, MPFR_RNDN);
      mpfr_set(im, o.im, MPFR_RNDN);
      rr = o.rr;
      ri = o.ri;
    }
    return *this;
  }

  CBall(CBall&& o) noexcept
    : rr(o.rr), ri(o.ri) {
    mpfr_init2(re, mpfr_get_prec(o.re));
    mpfr_init2(im, mpfr_get_prec(o.im));
    mpfr_swap(re, o.re);
    mpfr_swap(im, o.im);
    mpfr_set_zero(o.re, 0);
    mpfr_set_zero(o.im, 0);
    o.rr = 0;
    o.ri = 0;
  }

  CBall& operator=(CBall&& o) noexcept {
    if (this != &o) {
      mpfr_swap(re, o.re);
      mpfr_swap(im, o.im);
      rr = std::exchange(o.rr, 0.0);
      ri = std::exchange(o.ri, 0.0);
    }
    return *this;
  }

  void set_zero() {
    mpfr_set_zero(re, 0);
    mpfr_set_zero(im, 0);
    rr = 0.0;
    ri = 0.0;
  }

  void add(const CBall& o) {
    mpfr_add(re, re, o.re, MPFR_RNDN);
    mpfr_add(im, im, o.im, MPFR_RNDN);
    rr = cb::up_add(rr, o.rr);
    ri = cb::up_add(ri, o.ri);
  }

  void negate() {
    mpfr_neg(re, re, MPFR_RNDN);
    mpfr_neg(im, im, MPFR_RNDU);
  }

  // Complex product. Radius uses L1-norm bound (provably sufficient):
  // each component of a*b' is bounded by |a_L1| * |b_L1| where L1 includes
  // both centre and radius contributions. No division by 2 (conservative).
  // Centre-rounding term at wp included explicitly.
  void mul(const CBall& o, mpfr_prec_t wp) {
    using cb::up_add;

    const double aru = cb::fabs_upper(this->re);
    const double aiu = cb::fabs_upper(this->im);
    const double bru = cb::fabs_upper(o.re);
    const double biu = cb::fabs_upper(o.im);

    mpfr_t nre, nim, t1, t2;
    mpfr_init2(nre, wp); mpfr_init2(nim, wp);
    mpfr_init2(t1, wp); mpfr_init2(t2, wp);
    mpfr_mul(t1, re, o.re, MPFR_RNDN);
    mpfr_mul(t2, im, o.im, MPFR_RNDN);
    mpfr_sub(nre, t1, t2, MPFR_RNDN);
    mpfr_mul(t1, re, o.im, MPFR_RNDN);
    mpfr_mul(t2, im, o.re, MPFR_RNDN);
    mpfr_add(nim, t1, t2, MPFR_RNDN);

    const double a_l1 = up_add(up_add(aru, aiu), up_add(rr, ri));
    const double b_l1 = up_add(up_add(bru, biu), up_add(o.rr, o.ri));
    const double r_tot = up_add(a_l1 * b_l1,
        std::numeric_limits<double>::denorm_min());
    const double headroom = up_add(
        r_tot * 8.0 * std::numeric_limits<double>::epsilon(),
        std::numeric_limits<double>::denorm_min());

    mpfr_swap(re, nre); mpfr_swap(im, nim);

    rr = up_add(r_tot, headroom);
    ri = rr;

    mpfr_clear(nre); mpfr_clear(nim);
    mpfr_clear(t1); mpfr_clear(t2);
  }

  // Multiply by real coefficient ball (centre cf from mpfr, radius cfr).
  // Internal products use MPFR_RNDU for outward rounding of radii.
  void mul_real(const mpfr_t cf, double cfr, mpfr_prec_t wp) {
    const double cfu = cb::fabs_upper(cf);
    const double reu = cb::fabs_upper(this->re);
    const double iuu = cb::fabs_upper(this->im);

    mpfr_mul(re, re, cf, MPFR_RNDU);
    mpfr_mul(im, im, cf, MPFR_RNDU);

    const double grow = cb::up_add(
        rr * cfu + cfr * (reu + iuu),
        cfr * cfu);
    rr = grow;
    ri = grow;
  }
};

}  // namespace zetaforge
