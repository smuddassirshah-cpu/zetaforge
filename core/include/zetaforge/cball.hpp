#pragma once

// Complex interval (ball) arithmetic: an mpfr centre pair with componentwise
// double radii.
//
// Soundness contract: for every point (x, y) with |x - re| <= rr and
// |y - im| <= ri in each input, the corresponding true result lies inside the
// returned component radii. Derivation and term-by-term justification live in
// docs/MATHS.md under D7b.
//
// Decision notes, rev 5 rebuild (readiness review findings B1, B2, B3):
//
// - Radii are computed exclusively with the outward-rounding integer-exact
//   primitives in zetaforge/radius.hpp. The previous implementation performed
//   plain round-to-nearest double products and sums on the radius path, which
//   can round a bound DOWN and is precisely what those primitives exist to
//   prevent.
// - mul now carries the DEVIATION cross terms only. The previous bound was
//   L1(a) * L1(b) including both centre magnitudes, so the radius always
//   exceeded the output centre magnitude and every product ball contained
//   zero. That is sound but useless: no product could ever be signed, so sign
//   certification through this layer was structurally impossible.
// - Every mpfr operation on a component contributes an explicit centre-
//   rounding term (magnitude ceiling of that operation's result times
//   2^(1-wp)), summed outward: three per component in mul, one in add, one in
//   mul_real. add previously carried none at all, so two exact inputs whose
//   sum needed one more bit than the working precision produced a zero-radius
//   ball around a rounded centre. mul_real carried none either, and also
//   dropped the coefficient-radius times ball-radius cross term.
// - Centre magnitudes are directed ceilings. Taking fabs AFTER a round-up
//   conversion rounds negative values TOWARD zero and under-estimates the
//   magnitude, which understates every term it multiplies.
//
// Precision rule (rev 6, finding R6-1). A centre-rounding term is charged at
// the precision the result is actually rounded into, never at a nominal
// working precision the operation does not use. mul and mul_real build their
// results in fresh temporaries at wp and swap them in, so both charge at wp
// and leave every component stored at wp; add rounds in place and charges each
// component at that component's own stored precision. mul_real broke this rule
// through rev 5.

#include <cmath>
#include <limits>
#include <utility>
#include <mpfr.h>

#include "radius.hpp"

namespace zetaforge {

namespace cb {

// Upper bound on |x| as a double. Direction is chosen from the sign so the
// conversion always rounds AWAY from zero; a non-zero value never reports a
// zero magnitude.
inline double abs_upper(mpfr_srcptr x) noexcept {
  if (mpfr_zero_p(x)) {
    return 0.0;
  }
  const double v = mpfr_get_d(x, mpfr_sgn(x) >= 0 ? MPFR_RNDU : MPFR_RNDD);
  const double m = std::fabs(v);
  return m > 0.0 ? m : std::numeric_limits<double>::denorm_min();
}

// Unit round-off multiplier 2^(1-wp), exact where representable.
inline double round_unit(mpfr_prec_t wp) noexcept {
  const int k = 1 - static_cast<int>(wp);
  if (k < -1074) {
    return std::numeric_limits<double>::denorm_min();
  }
  return std::ldexp(1.0, k);
}

// Centre-rounding contribution of up to three mpfr operations whose results
// have the given magnitude ceilings. Summed and scaled outward.
inline double round_term(mpfr_prec_t wp, double m1, double m2 = 0.0,
                         double m3 = 0.0) noexcept {
  const double s = up_add(up_add(m1, m2), m3);
  return up_mul(s, round_unit(wp));
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

  CBall(const CBall& o) : rr(o.rr), ri(o.ri) {
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

  CBall(CBall&& o) noexcept : rr(o.rr), ri(o.ri) {
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

  // Negation is exact in mpfr: the radii are unchanged and no rounding term
  // arises.
  void negate() {
    mpfr_neg(re, re, MPFR_RNDN);
    mpfr_neg(im, im, MPFR_RNDN);
  }

  // Componentwise sum. Radii add outward, plus the centre rounding of the two
  // mpfr additions at the STORED precision of each component.
  void add(const CBall& o) {
    const mpfr_prec_t pre = mpfr_get_prec(re);
    const mpfr_prec_t pim = mpfr_get_prec(im);

    mpfr_add(re, re, o.re, MPFR_RNDN);
    mpfr_add(im, im, o.im, MPFR_RNDN);

    rr = inflate(up_add(up_add(rr, o.rr),
                        cb::round_term(pre, cb::abs_upper(re))));
    ri = inflate(up_add(up_add(ri, o.ri),
                        cb::round_term(pim, cb::abs_upper(im))));
  }

  // Complex product.
  //
  // Writing a = (ar + da) + i(ai + db) and b = (br + dc) + i(bi + dd) with
  // |da| <= rr_a, |db| <= ri_a, |dc| <= rr_b, |dd| <= ri_b, expanding and
  // dropping the centre product (which becomes the new centre) leaves
  //
  //   |dev Re| <= |ar|rr_b + |ai|ri_b + rr_a|br| + ri_a|bi|
  //               + rr_a rr_b + ri_a ri_b
  //   |dev Im| <= |ar|ri_b + |ai|rr_b + rr_a|bi| + ri_a|br|
  //               + rr_a ri_b + ri_a rr_b
  //
  // No centre magnitude product appears: that term IS the centre. See D7b.
  void mul(const CBall& o, mpfr_prec_t wp) {
    const double aru = cb::abs_upper(re);
    const double aiu = cb::abs_upper(im);
    const double bru = cb::abs_upper(o.re);
    const double biu = cb::abs_upper(o.im);
    const double rr_a = rr, ri_a = ri, rr_b = o.rr, ri_b = o.ri;

    mpfr_t t1, t2, u1, u2, nre, nim;
    mpfr_init2(t1, wp);
    mpfr_init2(t2, wp);
    mpfr_init2(u1, wp);
    mpfr_init2(u2, wp);
    mpfr_init2(nre, wp);
    mpfr_init2(nim, wp);

    mpfr_mul(t1, re, o.re, MPFR_RNDN);
    mpfr_mul(t2, im, o.im, MPFR_RNDN);
    mpfr_sub(nre, t1, t2, MPFR_RNDN);

    mpfr_mul(u1, re, o.im, MPFR_RNDN);
    mpfr_mul(u2, im, o.re, MPFR_RNDN);
    mpfr_add(nim, u1, u2, MPFR_RNDN);

    const double round_re = cb::round_term(
        wp, cb::abs_upper(t1), cb::abs_upper(t2), cb::abs_upper(nre));
    const double round_im = cb::round_term(
        wp, cb::abs_upper(u1), cb::abs_upper(u2), cb::abs_upper(nim));

    double dre = up_mul(aru, rr_b);
    dre = up_add(dre, up_mul(aiu, ri_b));
    dre = up_add(dre, up_mul(rr_a, bru));
    dre = up_add(dre, up_mul(ri_a, biu));
    dre = up_add(dre, up_mul(rr_a, rr_b));
    dre = up_add(dre, up_mul(ri_a, ri_b));
    dre = up_add(dre, round_re);

    double dim = up_mul(aru, ri_b);
    dim = up_add(dim, up_mul(aiu, rr_b));
    dim = up_add(dim, up_mul(rr_a, biu));
    dim = up_add(dim, up_mul(ri_a, bru));
    dim = up_add(dim, up_mul(rr_a, ri_b));
    dim = up_add(dim, up_mul(ri_a, rr_b));
    dim = up_add(dim, round_im);

    mpfr_swap(re, nre);
    mpfr_swap(im, nim);
    rr = inflate(dre);
    ri = inflate(dim);

    mpfr_clear(t1);
    mpfr_clear(t2);
    mpfr_clear(u1);
    mpfr_clear(u2);
    mpfr_clear(nre);
    mpfr_clear(nim);
  }

  // Multiply by a real ball (centre cf, radius cfr).
  //
  //   |dev Re| <= |re| cfr + rr |cf| + rr cfr    (and likewise for Im)
  //
  // The implementation uses the slightly coarser cfr(|re| + |im|) for both
  // components, which dominates the per-component term and keeps one shared
  // growth expression. The cfr * r_old term was absent before, and the centre
  // multiplications were rounded MPFR_RNDU on signed values, biasing the
  // centre with no radius to pay for it; both were fixed at rev 5.
  //
  // The products go into temporaries at wp and are swapped in, mirroring mul,
  // so the centre-rounding term is charged at the precision the result is
  // actually rounded into. Through rev 5 the multiplication ran in place at
  // each component's STORED precision while the term was still taken at wp:
  // for a stored precision below wp that under-reports the error committed,
  // and a 53-bit ball at centre 1 times a 200-bit coefficient 1 + 2^-100
  // returned a ball around 1.0 of radius ~2^-199 that excluded the true
  // product (R6-1; regression in test_cball layer C5).
  void mul_real(mpfr_srcptr cf, double cfr, mpfr_prec_t wp) {
    const double cfu = cb::abs_upper(cf);
    const double reu = cb::abs_upper(re);
    const double imu = cb::abs_upper(im);
    const double rr_old = rr, ri_old = ri;
    const double mag_sum = up_add(reu, imu);

    mpfr_t nre, nim;
    mpfr_init2(nre, wp);
    mpfr_init2(nim, wp);
    mpfr_mul(nre, re, cf, MPFR_RNDN);
    mpfr_mul(nim, im, cf, MPFR_RNDN);

    double gre = up_mul(cfr, mag_sum);
    gre = up_add(gre, up_mul(rr_old, cfu));
    gre = up_add(gre, up_mul(rr_old, cfr));
    gre = up_add(gre, cb::round_term(wp, cb::abs_upper(nre)));

    double gim = up_mul(cfr, mag_sum);
    gim = up_add(gim, up_mul(ri_old, cfu));
    gim = up_add(gim, up_mul(ri_old, cfr));
    gim = up_add(gim, cb::round_term(wp, cb::abs_upper(nim)));

    mpfr_swap(re, nre);
    mpfr_swap(im, nim);
    rr = inflate(gre);
    ri = inflate(gim);

    mpfr_clear(nre);
    mpfr_clear(nim);
  }
};

}  // namespace zetaforge
