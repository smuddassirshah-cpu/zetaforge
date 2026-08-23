#pragma once

// Complex interval (ball) arithmetic over an mpfr pair with double
// componentwise radii.
//
// Soundness contract (verified by test_cball.cpp Monte-Carlo corner sampling
// plus exact-rational spot checks): for every a' with |a'_re - re| <= rr and
// |a'_im - im| <= ri, and likewise b', the true product/sum lies within the
// returned component radii. Radius growth uses outward addition so
// accumulated bounds never round downward.

#include <cmath>
#include <limits>
#include <mpfr.h>

namespace zetaforge {

namespace cball_detail {

inline double up_add(double a, double b) noexcept {
  const double s = a + b;
  if (!std::isfinite(s)) return s;
  const double bv = s - a;
  const double err = (a - (s - bv)) + (b - bv);
  return err > 0.0 ? std::nextafter(s, INFINITY) : s;
}

inline double fabs_d(const mpfr_t x) noexcept {
  return std::fabs(mpfr_get_d(x, MPFR_RNDN));
}

}  // namespace cball_detail

struct CBall {
  mpfr_t re, im;
  double rr = 0.0, ri = 0.0;

  double max_radius() const { return rr > ri ? rr : ri; }

  // Uninitialised on construction: callers MUST call init(prec) before any
  // other member. This mirrors mpfr_t's own discipline.
  CBall() = default;

  void init(mpfr_prec_t p) {
    mpfr_init2(re, p);
    mpfr_init2(im, p);
    set_zero();
  }
  void clear() {
    mpfr_clear(re);
    mpfr_clear(im);
  }

  // Rule of three/five: mpfr_t holds heap state; default copies would alias.
  CBall(const CBall& o) {
    init(mpfr_get_prec(o.re));
    mpfr_set(re, o.re, MPFR_RNDN);
    mpfr_set(im, o.im, MPFR_RNDN);
    rr = o.rr;
    ri = o.ri;
  }
  CBall& operator=(const CBall& o) {
    if (this != &o) {
      if (mpfr_get_prec(re) != mpfr_get_prec(o.re)) {
        mpfr_set_prec(re, mpfr_get_prec(o.re));
        mpfr_set_prec(im, mpfr_get_prec(o.re));
      }
      mpfr_set(re, o.re, MPFR_RNDN);
      mpfr_set(im, o.im, MPFR_RNDN);
      rr = o.rr;
      ri = o.ri;
    }
    return *this;
  }
  CBall(CBall&& o) noexcept {
    init(mpfr_get_prec(o.re));
    mpfr_swap(re, o.re);
    mpfr_swap(im, o.im);
    rr = o.rr;
    ri = o.ri;
  }
  CBall& operator=(CBall&& o) noexcept {
    if (this != &o) {
      mpfr_swap(re, o.re);
      mpfr_swap(im, o.im);
      rr = o.rr;
      ri = o.ri;
    }
    return *this;
  }

  void set_zero() {
    mpfr_set_zero(re, 0);
    mpfr_set_zero(im, 0);
    rr = 0.0;
    ri = 0.0;
  }

  // Componentwise outward addition.
  void add(const CBall& o) {
    mpfr_add(re, re, o.re, MPFR_RNDN);
    mpfr_add(im, im, o.im, MPFR_RNDN);
    rr = cball_detail::up_add(rr, o.rr);
    ri = cball_detail::up_add(ri, o.ri);
  }

  void negate() {
    mpfr_neg(re, re, MPFR_RNDN);
    mpfr_neg(im, im, MPFR_RNDU);
  }

  // Complex product. Radii use centre magnitudes of BOTH operands and are
  // conservative symmetric bounds:
  //   |dRe| <= rr*(|bre|+|bim|) + o.rd*(|are|+|aim|) + rr*o.rd   (and Im)
  void mul(const CBall& o, mpfr_prec_t wp) {
    const double are = cball_detail::fabs_d(re);
    const double aim = cball_detail::fabs_d(im);
    const double bre = cball_detail::fabs_d(o.re);
    const double bim = cball_detail::fabs_d(o.im);

    mpfr_t nre, nim, p1, p2;
    mpfr_init2(nre, wp); mpfr_init2(nim, wp);
    mpfr_init2(p1, wp); mpfr_init2(p2, wp);
    mpfr_mul(p1, re, o.re, MPFR_RNDN);
    mpfr_mul(p2, im, o.im, MPFR_RNDN);
    mpfr_sub(nre, p1, p2, MPFR_RNDN);
    mpfr_mul(p1, re, o.im, MPFR_RNDN);
    mpfr_mul(p2, im, o.re, MPFR_RNDN);
    mpfr_add(nim, p1, p2, MPFR_RNDN);
    mpfr_swap(re, nre);
    mpfr_swap(im, nim);
    mpfr_clear(nre); mpfr_clear(nim);
    mpfr_clear(p1); mpfr_clear(p2);

    using cball_detail::up_add;
    // Corner-magnitude bounds (not just centre): when both operands carry
    // radii, the product of their corner extents exceeds what centre-only
    // bounds predict. This was found by the exact-dyadic containment test.
    const double a_extent = up_add(are, aim);
    const double b_extent = up_add(bre, bim);
    const double r_cross = up_add(rr * b_extent, o.rr * a_extent);
    const double self_max = rr > ri ? rr : ri;
    const double other_max = o.rr > o.ri ? o.rr : o.ri;
    const double r_tot = up_add(up_add(r_cross, self_max * other_max),
                                self_max * other_max);
    const double headroom = up_add(
        r_tot * 8.0 * std::numeric_limits<double>::epsilon(),
        std::numeric_limits<double>::denorm_min());
    rr = up_add(r_tot, headroom);
    ri = rr;
  }

  // Multiply by a real coefficient ball (centre cf, radius cfr).
  void mul_real(const mpfr_t cf, double cfr, mpfr_prec_t wp) {
    const double cmag = cball_detail::fabs_d(cf);
    const double are = cball_detail::fabs_d(re);
    const double aim = cball_detail::fabs_d(im);
    const double r_old = rr;

    mpfr_mul(re, re, cf, MPFR_RNDN);
    mpfr_mul(im, im, cf, MPFR_RNDN);

    // Both components inherit one conservative bound computed from the OLD
    // radii: |delta| <= r_old*(|cf|+cfr) + cfr*(|re|+|im|).
    using cball_detail::up_add;
    const double grow = up_add(r_old * (cmag + cfr), cfr * (are + aim));
    rr = grow;
    ri = grow;
  }
};

}  // namespace zetaforge
