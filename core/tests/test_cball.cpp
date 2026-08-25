// Complex-ball arithmetic tests.
//
// Decision notes.
//
// Every layer runs against all three operations (mul, add, mul_real). Through
// rev 5 only mul was covered, so mul_real's precision defect (R6-1) and add's
// bounds were untested by C1, C2 and C4 alike; the parity is the point of this
// revision (R6-2).
//
// Stored precision and working precision are drawn INDEPENDENTLY from
// {53, 128, 160, 256} on the check.hpp seed stream. A suite that always used
// stored == wp could not see a term charged at the wrong precision, which is
// exactly the defect C5 pins.
//
// All reference arithmetic is exact. Ball centres are dyadic m * 2^e with
// |m| < 2^20, so they are representable without loss at every precision in the
// pool, and every reference quantity is computed in mpfr at kRefPrec where
// those dyadics and their products are exact. No comparison passes through a
// double.
//
// Layers:
//   C1 exact dyadic corner containment: every corner of the true result set
//      must lie inside the claimed output box. This is the under-radius
//      failure the radius arithmetic exists to prevent.
//   C2 cut detection: shrinking the claimed radii by 0.9 or 0.5 must push at
//      least one exact corner outside the box. A suite that passes with and
//      without a correct radius is worthless (stage 2 gate doctrine).
//   C3 exact-rational spot checks on small integer complex inputs.
//   C4 tightness: the claimed radius must dominate the exact deviation bound
//      of MATHS.md D7b and must not exceed four times the full D7b bound
//      (deviation plus centre rounding). Corner containment cannot see an
//      over-wide radius; this layer is what fails a centre-inclusive one.
//   C5 precision rule: the centre-rounding term is charged at the precision
//      the result is actually rounded into (R6-1).

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

#include "check.hpp"
#include "zetaforge/cball.hpp"

using zetaforge::CBall;

namespace {

constexpr mpfr_prec_t kRefPrec = 600;
constexpr mpfr_prec_t kPrecPool[4] = {53, 128, 160, 256};
constexpr int kTrials = 300;

mpfr_prec_t pick_prec() {
  return kPrecPool[::zftest::rng().next() & 3];
}

// Exact dyadic value m * 2^e. |m| < 2^20 keeps it representable at 53 bits.
struct Dy {
  long long m;
  int e;
};

Dy pick_centre() {
  const uint64_t r = ::zftest::rng().next();
  long long m = static_cast<long long>(r % 1048576ULL);   // < 2^20
  if ((r >> 20) & 1) m = -m;
  const int e = -static_cast<int>((r >> 21) % 71ULL);     // 2^-70 .. 2^0
  return {m, e};
}

// Radii are exact dyadic doubles: p * 2^f with p < 2^10.
double pick_radius() {
  const uint64_t r = ::zftest::rng().next();
  const int p = static_cast<int>(r % 1024ULL);
  const int f = -static_cast<int>((r >> 10) % 61ULL);     // 2^-60 .. 2^0
  return std::ldexp(static_cast<double>(p), f);
}

// RAII mpfr scalar at the reference precision.
struct Mp {
  mpfr_t v;
  Mp() { mpfr_init2(v, kRefPrec); mpfr_set_zero(v, 0); }
  explicit Mp(double d) { mpfr_init2(v, kRefPrec); mpfr_set_d(v, d, MPFR_RNDN); }
  explicit Mp(const Dy& d) {
    mpfr_init2(v, kRefPrec);
    mpfr_set_si(v, d.m, MPFR_RNDN);
    mpfr_mul_2si(v, v, d.e, MPFR_RNDN);
  }
  Mp(const Mp&) = delete;
  Mp& operator=(const Mp&) = delete;
  ~Mp() { mpfr_clear(v); }
};

// b = ball with dyadic centre and exact dyadic radii, at stored precision p.
CBall make_ball(mpfr_prec_t p, const Dy& cre, const Dy& cim, double rr,
                double ri) {
  CBall b(p);
  const Mp x(cre), y(cim);
  // Exact: |m| < 2^20 and p >= 53.
  mpfr_set(b.re, x.v, MPFR_RNDN);
  mpfr_set(b.im, y.v, MPFR_RNDN);
  b.rr = rr;
  b.ri = ri;
  return b;
}

// |value - centre| <= box, in full reference precision.
bool within(mpfr_srcptr value, mpfr_srcptr centre, double box) {
  Mp d;
  mpfr_sub(d.v, value, centre, MPFR_RNDN);   // exact at kRefPrec here
  mpfr_abs(d.v, d.v, MPFR_RNDN);
  return mpfr_cmp_d(d.v, box) <= 0;
}

// ---- exact corner enumeration -------------------------------------------
//
// Each callback receives the exact result components of one corner of the true
// input set. Every corner coordinate is an exact dyadic, so every product and
// sum below is exact at kRefPrec.

template <typename F>
void corners_mul(const Dy& ar, const Dy& ai, double rr_a, double ri_a,
                 const Dy& br, const Dy& bi, double rr_b, double ri_b, F&& f) {
  const Mp AR(ar), AI(ai), BR(br), BI(bi);
  const Mp RA(rr_a), IA(ri_a), RB(rr_b), IB(ri_b);
  Mp x, y, u, v, t1, t2, re, im;
  for (int s1 = -1; s1 <= 1; s1 += 2)
    for (int s2 = -1; s2 <= 1; s2 += 2)
      for (int s3 = -1; s3 <= 1; s3 += 2)
        for (int s4 = -1; s4 <= 1; s4 += 2) {
          (s1 > 0 ? mpfr_add : mpfr_sub)(x.v, AR.v, RA.v, MPFR_RNDN);
          (s2 > 0 ? mpfr_add : mpfr_sub)(y.v, AI.v, IA.v, MPFR_RNDN);
          (s3 > 0 ? mpfr_add : mpfr_sub)(u.v, BR.v, RB.v, MPFR_RNDN);
          (s4 > 0 ? mpfr_add : mpfr_sub)(v.v, BI.v, IB.v, MPFR_RNDN);
          mpfr_mul(t1.v, x.v, u.v, MPFR_RNDN);
          mpfr_mul(t2.v, y.v, v.v, MPFR_RNDN);
          mpfr_sub(re.v, t1.v, t2.v, MPFR_RNDN);
          mpfr_mul(t1.v, x.v, v.v, MPFR_RNDN);
          mpfr_mul(t2.v, y.v, u.v, MPFR_RNDN);
          mpfr_add(im.v, t1.v, t2.v, MPFR_RNDN);
          f(re.v, im.v);
        }
}

template <typename F>
void corners_add(const Dy& ar, const Dy& ai, double rr_a, double ri_a,
                 const Dy& br, const Dy& bi, double rr_b, double ri_b, F&& f) {
  const Mp AR(ar), AI(ai), BR(br), BI(bi);
  const Mp RA(rr_a), IA(ri_a), RB(rr_b), IB(ri_b);
  Mp x, y, u, v, re, im;
  for (int s1 = -1; s1 <= 1; s1 += 2)
    for (int s2 = -1; s2 <= 1; s2 += 2)
      for (int s3 = -1; s3 <= 1; s3 += 2)
        for (int s4 = -1; s4 <= 1; s4 += 2) {
          (s1 > 0 ? mpfr_add : mpfr_sub)(x.v, AR.v, RA.v, MPFR_RNDN);
          (s2 > 0 ? mpfr_add : mpfr_sub)(y.v, AI.v, IA.v, MPFR_RNDN);
          (s3 > 0 ? mpfr_add : mpfr_sub)(u.v, BR.v, RB.v, MPFR_RNDN);
          (s4 > 0 ? mpfr_add : mpfr_sub)(v.v, BI.v, IB.v, MPFR_RNDN);
          mpfr_add(re.v, x.v, u.v, MPFR_RNDN);
          mpfr_add(im.v, y.v, v.v, MPFR_RNDN);
          f(re.v, im.v);
        }
}

template <typename F>
void corners_mul_real(const Dy& ar, const Dy& ai, double rr_a, double ri_a,
                      const Dy& cf, double cfr, F&& f) {
  const Mp AR(ar), AI(ai), CF(cf);
  const Mp RA(rr_a), IA(ri_a), CR(cfr);
  Mp x, y, c, re, im;
  for (int s1 = -1; s1 <= 1; s1 += 2)
    for (int s2 = -1; s2 <= 1; s2 += 2)
      for (int s3 = -1; s3 <= 1; s3 += 2) {
        (s1 > 0 ? mpfr_add : mpfr_sub)(x.v, AR.v, RA.v, MPFR_RNDN);
        (s2 > 0 ? mpfr_add : mpfr_sub)(y.v, AI.v, IA.v, MPFR_RNDN);
        (s3 > 0 ? mpfr_add : mpfr_sub)(c.v, CF.v, CR.v, MPFR_RNDN);
        mpfr_mul(re.v, x.v, c.v, MPFR_RNDN);
        mpfr_mul(im.v, y.v, c.v, MPFR_RNDN);
        f(re.v, im.v);
      }
}

// ---- exact D7b bounds, transcribed independently of the implementation ----

void acc(mpfr_ptr out, mpfr_srcptr a, mpfr_srcptr b, mpfr_ptr tmp) {
  mpfr_mul(tmp, a, b, MPFR_RNDN);
  mpfr_add(out, out, tmp, MPFR_RNDN);
}

// Deviation part of D7b for mul, plus the three-operation centre-rounding term.
void bound_mul(const Dy& ar, const Dy& ai, double rr_a, double ri_a,
               const Dy& br, const Dy& bi, double rr_b, double ri_b,
               mpfr_prec_t wp, mpfr_ptr dev_re, mpfr_ptr dev_im,
               mpfr_ptr full_re, mpfr_ptr full_im) {
  const Mp AR(ar), AI(ai), BR(br), BI(bi);
  const Mp RA(rr_a), IA(ri_a), RB(rr_b), IB(ri_b);
  Mp aar, aai, abr, abi, tmp;
  mpfr_abs(aar.v, AR.v, MPFR_RNDN);
  mpfr_abs(aai.v, AI.v, MPFR_RNDN);
  mpfr_abs(abr.v, BR.v, MPFR_RNDN);
  mpfr_abs(abi.v, BI.v, MPFR_RNDN);

  mpfr_set_zero(dev_re, 0);
  acc(dev_re, aar.v, RB.v, tmp.v);
  acc(dev_re, aai.v, IB.v, tmp.v);
  acc(dev_re, RA.v, abr.v, tmp.v);
  acc(dev_re, IA.v, abi.v, tmp.v);
  acc(dev_re, RA.v, RB.v, tmp.v);
  acc(dev_re, IA.v, IB.v, tmp.v);

  mpfr_set_zero(dev_im, 0);
  acc(dev_im, aar.v, IB.v, tmp.v);
  acc(dev_im, aai.v, RB.v, tmp.v);
  acc(dev_im, RA.v, abi.v, tmp.v);
  acc(dev_im, IA.v, abr.v, tmp.v);
  acc(dev_im, RA.v, IB.v, tmp.v);
  acc(dev_im, IA.v, RB.v, tmp.v);

  // Centre rounding: three mpfr operations per component, each charged
  // |result| * 2^(1-wp) at the precision the result is rounded into.
  Mp t1, t2, nre, nim, r;
  mpfr_mul(t1.v, AR.v, BR.v, MPFR_RNDN);
  mpfr_mul(t2.v, AI.v, BI.v, MPFR_RNDN);
  mpfr_sub(nre.v, t1.v, t2.v, MPFR_RNDN);
  mpfr_abs(r.v, t1.v, MPFR_RNDN);
  mpfr_abs(t1.v, t2.v, MPFR_RNDN);
  mpfr_add(r.v, r.v, t1.v, MPFR_RNDN);
  mpfr_abs(t1.v, nre.v, MPFR_RNDN);
  mpfr_add(r.v, r.v, t1.v, MPFR_RNDN);
  mpfr_mul_2si(r.v, r.v, 1 - static_cast<int>(wp), MPFR_RNDN);
  mpfr_add(full_re, dev_re, r.v, MPFR_RNDN);

  mpfr_mul(t1.v, AR.v, BI.v, MPFR_RNDN);
  mpfr_mul(t2.v, AI.v, BR.v, MPFR_RNDN);
  mpfr_add(nim.v, t1.v, t2.v, MPFR_RNDN);
  mpfr_abs(r.v, t1.v, MPFR_RNDN);
  mpfr_abs(t1.v, t2.v, MPFR_RNDN);
  mpfr_add(r.v, r.v, t1.v, MPFR_RNDN);
  mpfr_abs(t1.v, nim.v, MPFR_RNDN);
  mpfr_add(r.v, r.v, t1.v, MPFR_RNDN);
  mpfr_mul_2si(r.v, r.v, 1 - static_cast<int>(wp), MPFR_RNDN);
  mpfr_add(full_im, dev_im, r.v, MPFR_RNDN);
}

void bound_add(const Dy& ar, const Dy& ai, double rr_a, double ri_a,
               const Dy& br, const Dy& bi, double rr_b, double ri_b,
               mpfr_prec_t stored, mpfr_ptr dev_re, mpfr_ptr dev_im,
               mpfr_ptr full_re, mpfr_ptr full_im) {
  const Mp AR(ar), AI(ai), BR(br), BI(bi);
  mpfr_set_d(dev_re, rr_a, MPFR_RNDN);
  mpfr_add_d(dev_re, dev_re, rr_b, MPFR_RNDN);
  mpfr_set_d(dev_im, ri_a, MPFR_RNDN);
  mpfr_add_d(dev_im, dev_im, ri_b, MPFR_RNDN);

  // One mpfr addition per component, charged at the STORED precision, which
  // is the precision add rounds into.
  Mp s, r;
  mpfr_add(s.v, AR.v, BR.v, MPFR_RNDN);
  mpfr_abs(r.v, s.v, MPFR_RNDN);
  mpfr_mul_2si(r.v, r.v, 1 - static_cast<int>(stored), MPFR_RNDN);
  mpfr_add(full_re, dev_re, r.v, MPFR_RNDN);

  mpfr_add(s.v, AI.v, BI.v, MPFR_RNDN);
  mpfr_abs(r.v, s.v, MPFR_RNDN);
  mpfr_mul_2si(r.v, r.v, 1 - static_cast<int>(stored), MPFR_RNDN);
  mpfr_add(full_im, dev_im, r.v, MPFR_RNDN);
}

// dev_* is the exact per-component deviation bound of D7b; full_* adds the
// one-operation centre rounding at wp. Both are attained at a corner, so the
// implementation has no slack to give away here and a 0.9x cut is visible.
void bound_mul_real(const Dy& ar, const Dy& ai, double rr_a, double ri_a,
                    const Dy& cf, double cfr, mpfr_prec_t wp,
                    mpfr_ptr dev_re, mpfr_ptr dev_im,
                    mpfr_ptr full_re, mpfr_ptr full_im) {
  const Mp AR(ar), AI(ai), CF(cf), RA(rr_a), IA(ri_a), CR(cfr);
  Mp aar, aai, acf, tmp;
  mpfr_abs(aar.v, AR.v, MPFR_RNDN);
  mpfr_abs(aai.v, AI.v, MPFR_RNDN);
  mpfr_abs(acf.v, CF.v, MPFR_RNDN);

  mpfr_set_zero(dev_re, 0);
  acc(dev_re, aar.v, CR.v, tmp.v);
  acc(dev_re, RA.v, acf.v, tmp.v);
  acc(dev_re, RA.v, CR.v, tmp.v);

  mpfr_set_zero(dev_im, 0);
  acc(dev_im, aai.v, CR.v, tmp.v);
  acc(dev_im, IA.v, acf.v, tmp.v);
  acc(dev_im, IA.v, CR.v, tmp.v);

  Mp prod, r;

  mpfr_set(full_re, dev_re, MPFR_RNDN);
  mpfr_mul(prod.v, AR.v, CF.v, MPFR_RNDN);
  mpfr_abs(r.v, prod.v, MPFR_RNDN);
  mpfr_mul_2si(r.v, r.v, 1 - static_cast<int>(wp), MPFR_RNDN);
  mpfr_add(full_re, full_re, r.v, MPFR_RNDN);

  mpfr_set(full_im, dev_im, MPFR_RNDN);
  mpfr_mul(prod.v, AI.v, CF.v, MPFR_RNDN);
  mpfr_abs(r.v, prod.v, MPFR_RNDN);
  mpfr_mul_2si(r.v, r.v, 1 - static_cast<int>(wp), MPFR_RNDN);
  mpfr_add(full_im, full_im, r.v, MPFR_RNDN);
}

// claimed >= dev (soundness) and claimed <= 4 * full (tightness).
bool tight(double claimed, mpfr_srcptr dev, mpfr_srcptr full) {
  if (mpfr_cmp_d(dev, claimed) > 0) return false;
  Mp cap;
  mpfr_mul_ui(cap.v, full, 4, MPFR_RNDN);
  return mpfr_cmp_d(cap.v, claimed) >= 0;
}

}  // namespace

int main() {
  std::fprintf(stdout, "SEED %llx\n",
               static_cast<unsigned long long>(::zftest::current_seed()));

  // ---- C3: exact integer spot checks -------------------------------------
  {
    constexpr mpfr_prec_t P = 160;
    CBall a = make_ball(P, {1, 0}, {1, -3}, 0.0, 0.0);       // 1 + 0.125i
    CBall b = make_ball(P, {2, 0}, {-1, -2}, 0.0, 0.0);      // 2 - 0.25i
    a.mul(b, P);                                             // 2.03125 + 0i
    ZF_CHECK(std::fabs(mpfr_get_d(a.re, MPFR_RNDN) - 2.03125) < 1e-40);
    ZF_CHECK(std::fabs(mpfr_get_d(a.im, MPFR_RNDN)) < 1e-40);

    CBall c = make_ball(P, {7, -1}, {-9, -2}, 0.0, 0.0);     // 3.5 - 2.25i
    CBall d = make_ball(P, {3, -2}, {3, -1}, 0.0, 0.0);      // 0.75 + 1.5i
    c.add(d);                                                // 4.25 - 0.75i
    ZF_CHECK(std::fabs(mpfr_get_d(c.re, MPFR_RNDN) - 4.25) < 1e-40);
    ZF_CHECK(std::fabs(mpfr_get_d(c.im, MPFR_RNDN) + 0.75) < 1e-40);
  }

  // ---- C1 and C4: containment and tightness, all three operations ---------
  int c1_fail_mul = 0, c1_fail_add = 0, c1_fail_mr = 0;
  int c4_fail_mul = 0, c4_fail_add = 0, c4_fail_mr = 0;
  int mixed_mul = 0, mixed_add = 0, mixed_mr = 0;

  for (int trial = 0; trial < kTrials; ++trial) {
    const mpfr_prec_t sp = pick_prec();
    const mpfr_prec_t wp = pick_prec();
    const Dy ar = pick_centre(), ai = pick_centre();
    const Dy br = pick_centre(), bi = pick_centre();
    const double rr_a = pick_radius(), ri_a = pick_radius();
    const double rr_b = pick_radius(), ri_b = pick_radius();

    Mp dev_re, dev_im, full_re, full_im;

    // -- mul --------------------------------------------------------------
    {
      if (sp != wp) ++mixed_mul;
      CBall a = make_ball(sp, ar, ai, rr_a, ri_a);
      const CBall b = make_ball(sp, br, bi, rr_b, ri_b);
      a.mul(b, wp);
      corners_mul(ar, ai, rr_a, ri_a, br, bi, rr_b, ri_b,
                  [&](mpfr_srcptr tre, mpfr_srcptr tim) {
                    if (!within(tre, a.re, a.rr) || !within(tim, a.im, a.ri)) {
                      if (c1_fail_mul == 0) {
                        std::printf("C1FAIL mul sp=%d wp=%d\n",
                                    static_cast<int>(sp), static_cast<int>(wp));
                      }
                      ++c1_fail_mul;
                    }
                  });
      bound_mul(ar, ai, rr_a, ri_a, br, bi, rr_b, ri_b, wp,
                dev_re.v, dev_im.v, full_re.v, full_im.v);
      if (!tight(a.rr, dev_re.v, full_re.v) ||
          !tight(a.ri, dev_im.v, full_im.v)) {
        if (c4_fail_mul == 0) {
          std::printf("C4FAIL mul sp=%d wp=%d rr=%.17g ri=%.17g\n",
                      static_cast<int>(sp), static_cast<int>(wp), a.rr, a.ri);
        }
        ++c4_fail_mul;
      }
    }

    // -- add ---------------------------------------------------------------
    {
      if (sp != wp) ++mixed_add;
      CBall a = make_ball(sp, ar, ai, rr_a, ri_a);
      const CBall b = make_ball(wp, br, bi, rr_b, ri_b);   // operand at wp
      a.add(b);
      corners_add(ar, ai, rr_a, ri_a, br, bi, rr_b, ri_b,
                  [&](mpfr_srcptr tre, mpfr_srcptr tim) {
                    if (!within(tre, a.re, a.rr) || !within(tim, a.im, a.ri)) {
                      if (c1_fail_add == 0) {
                        std::printf("C1FAIL add sp=%d wp=%d\n",
                                    static_cast<int>(sp), static_cast<int>(wp));
                      }
                      ++c1_fail_add;
                    }
                  });
      bound_add(ar, ai, rr_a, ri_a, br, bi, rr_b, ri_b, sp,
                dev_re.v, dev_im.v, full_re.v, full_im.v);
      if (!tight(a.rr, dev_re.v, full_re.v) ||
          !tight(a.ri, dev_im.v, full_im.v)) {
        if (c4_fail_add == 0) {
          std::printf("C4FAIL add sp=%d wp=%d rr=%.17g ri=%.17g\n",
                      static_cast<int>(sp), static_cast<int>(wp), a.rr, a.ri);
        }
        ++c4_fail_add;
      }
    }

    // -- mul_real -----------------------------------------------------------
    {
      if (sp != wp) ++mixed_mr;
      const Dy cf = br;
      const double cfr = rr_b;
      CBall a = make_ball(sp, ar, ai, rr_a, ri_a);
      const Mp CF(cf);
      a.mul_real(CF.v, cfr, wp);
      corners_mul_real(ar, ai, rr_a, ri_a, cf, cfr,
                       [&](mpfr_srcptr tre, mpfr_srcptr tim) {
                         if (!within(tre, a.re, a.rr) ||
                             !within(tim, a.im, a.ri)) {
                           if (c1_fail_mr == 0) {
                             std::printf("C1FAIL mul_real sp=%d wp=%d\n",
                                         static_cast<int>(sp),
                                         static_cast<int>(wp));
                           }
                           ++c1_fail_mr;
                         }
                       });
      bound_mul_real(ar, ai, rr_a, ri_a, cf, cfr, wp,
                     dev_re.v, dev_im.v, full_re.v, full_im.v);
      if (!tight(a.rr, dev_re.v, full_re.v) ||
          !tight(a.ri, dev_im.v, full_im.v)) {
        if (c4_fail_mr == 0) {
          std::printf("C4FAIL mul_real sp=%d wp=%d rr=%.17g ri=%.17g\n",
                      static_cast<int>(sp), static_cast<int>(wp), a.rr, a.ri);
        }
        ++c4_fail_mr;
      }
      ZF_CHECK(mpfr_get_prec(a.re) == wp);
      ZF_CHECK(mpfr_get_prec(a.im) == wp);
    }
  }

  ZF_CHECK(c1_fail_mul == 0);
  ZF_CHECK(c1_fail_add == 0);
  ZF_CHECK(c1_fail_mr == 0);
  ZF_CHECK(c4_fail_mul == 0);
  ZF_CHECK(c4_fail_add == 0);
  ZF_CHECK(c4_fail_mr == 0);
  // Stored precision != wp must actually be exercised, or the layer is blind
  // to a term charged at the wrong precision.
  ZF_CHECK(mixed_mul > 0);
  ZF_CHECK(mixed_add > 0);
  ZF_CHECK(mixed_mr > 0);
  std::printf("C1_TRIALS %d mixed mul=%d add=%d mul_real=%d\n",
              kTrials, mixed_mul, mixed_add, mixed_mr);
  std::printf("C1_FAILURES mul=%d add=%d mul_real=%d\n",
              c1_fail_mul, c1_fail_add, c1_fail_mr);
  std::printf("C4_FAILURES mul=%d add=%d mul_real=%d\n",
              c4_fail_mul, c4_fail_add, c4_fail_mr);

  // ---- C2: cut detection, all three operations ---------------------------
  // Fixtures where the deviation part dominates the centre rounding, swept
  // over the whole precision grid so stored != wp is covered exhaustively
  // here rather than by sampling. A cut that no corner can see would mean the
  // claimed radius has slack to give away, which is the rev 1 defect (B1).
  {
    const Dy ar{5, -1}, ai{-3, -1};                   // 2.5, -1.5
    const Dy br{7, -2}, bi{9, -2};                    // 1.75, 2.25
    const double rr_a = std::ldexp(1.0, -4), ri_a = std::ldexp(1.0, -5);
    const double rr_b = std::ldexp(1.0, -3), ri_b = std::ldexp(1.0, -6);

    int det_mul_9 = 0, det_mul_5 = 0, det_add_9 = 0, det_add_5 = 0;
    int det_mr_9 = 0, det_mr_5 = 0, grid = 0;

    for (int si = 0; si < 4; ++si) {
      for (int wi = 0; wi < 4; ++wi) {
        const mpfr_prec_t sp = kPrecPool[si];
        const mpfr_prec_t wp = kPrecPool[wi];
        ++grid;

        auto any_outside = [](auto&& enumerate, const CBall& out, double cut) {
          bool outside = false;
          enumerate([&](mpfr_srcptr tre, mpfr_srcptr tim) {
            if (outside) return;
            if (!within(tre, out.re, out.rr * cut) ||
                !within(tim, out.im, out.ri * cut)) {
              outside = true;
            }
          });
          return outside;
        };

        {
          CBall a = make_ball(sp, ar, ai, rr_a, ri_a);
          const CBall b = make_ball(sp, br, bi, rr_b, ri_b);
          a.mul(b, wp);
          auto en = [&](auto&& f) {
            corners_mul(ar, ai, rr_a, ri_a, br, bi, rr_b, ri_b, f);
          };
          ZF_CHECK(!any_outside(en, a, 1.0));
          det_mul_9 += any_outside(en, a, 0.9) ? 1 : 0;
          det_mul_5 += any_outside(en, a, 0.5) ? 1 : 0;
        }
        {
          CBall a = make_ball(sp, ar, ai, rr_a, ri_a);
          const CBall b = make_ball(wp, br, bi, rr_b, ri_b);
          a.add(b);
          auto en = [&](auto&& f) {
            corners_add(ar, ai, rr_a, ri_a, br, bi, rr_b, ri_b, f);
          };
          ZF_CHECK(!any_outside(en, a, 1.0));
          det_add_9 += any_outside(en, a, 0.9) ? 1 : 0;
          det_add_5 += any_outside(en, a, 0.5) ? 1 : 0;
        }
        {
          CBall a = make_ball(sp, ar, ai, rr_a, ri_a);
          const Mp CF(br);
          a.mul_real(CF.v, rr_b, wp);
          auto en = [&](auto&& f) {
            corners_mul_real(ar, ai, rr_a, ri_a, br, rr_b, f);
          };
          ZF_CHECK(!any_outside(en, a, 1.0));
          det_mr_9 += any_outside(en, a, 0.9) ? 1 : 0;
          det_mr_5 += any_outside(en, a, 0.5) ? 1 : 0;
        }
      }
    }

    ZF_CHECK(det_mul_9 == grid);
    ZF_CHECK(det_mul_5 == grid);
    ZF_CHECK(det_add_9 == grid);
    ZF_CHECK(det_add_5 == grid);
    ZF_CHECK(det_mr_9 == grid);
    ZF_CHECK(det_mr_5 == grid);
    std::printf("C2_DETECT grid=%d mul 0.9=%d 0.5=%d | add 0.9=%d 0.5=%d | "
                "mul_real 0.9=%d 0.5=%d\n",
                grid, det_mul_9, det_mul_5, det_add_9, det_add_5,
                det_mr_9, det_mr_5);
  }

  // ---- C4 signability: a product far from the origin must not contain zero -
  // The direct statement of review finding B1: a centre-inclusive radius makes
  // every product ball straddle zero, so nothing downstream can ever be signed.
  {
    constexpr mpfr_prec_t P = 160;
    const Dy ar{300, -6}, ai{-200, -6}, br{500, -6}, bi{700, -6};
    const double rr_a = 3.0 / 64.0, ri_a = 2.0 / 64.0;
    const double rr_b = 4.0 / 64.0, ri_b = 1.0 / 64.0;
    CBall a = make_ball(P, ar, ai, rr_a, ri_a);
    const CBall b = make_ball(P, br, bi, rr_b, ri_b);
    a.mul(b, P);
    const double cre = std::fabs(mpfr_get_d(a.re, MPFR_RNDN));
    const double cim = std::fabs(mpfr_get_d(a.im, MPFR_RNDN));
    ZF_CHECK(cre > a.rr);
    ZF_CHECK(cim > a.ri);
    std::printf("C4_SIGNABLE re=%d im=%d\n",
                static_cast<int>(cre > a.rr), static_cast<int>(cim > a.ri));
  }

  // ---- C5: the centre-rounding term is taken at the precision the result is
  // actually rounded into (R6-1) ------------------------------------------
  // mul_real rounded its products in place, at each component's STORED
  // precision, while charging the rounding term at wp. With a stored precision
  // below wp the charge under-reports the error actually committed: a 53-bit
  // ball at centre 1 multiplied by a 200-bit coefficient carrying 100 bits of
  // new information returned a ball around 1.0 whose radius was ~2^-199, while
  // the true product 1 + 2^-100 sat 2^-100 away.
  {
    constexpr mpfr_prec_t kStored = 53;
    constexpr mpfr_prec_t kWp = 200;

    CBall a(kStored);
    mpfr_set_d(a.re, 1.0, MPFR_RNDN);
    mpfr_set_d(a.im, 0.0, MPFR_RNDN);
    a.rr = 0.0;
    a.ri = 0.0;

    mpfr_t cf, eps;
    mpfr_init2(cf, kWp);
    mpfr_init2(eps, kWp);
    mpfr_set_ui(cf, 1, MPFR_RNDN);
    mpfr_set_ui(eps, 1, MPFR_RNDN);
    mpfr_div_2si(eps, eps, 100, MPFR_RNDN);   // 2^-100, exact at 200 bits
    mpfr_add(cf, cf, eps, MPFR_RNDN);         // 1 + 2^-100, exact at 200 bits

    a.mul_real(cf, 0.0, kWp);

    mpfr_t truth, dev;
    mpfr_init2(truth, 300);
    mpfr_init2(dev, 300);
    mpfr_set_ui(truth, 1, MPFR_RNDN);
    mpfr_set_ui(dev, 1, MPFR_RNDN);
    mpfr_div_2si(dev, dev, 100, MPFR_RNDN);
    mpfr_add(truth, truth, dev, MPFR_RNDN);
    mpfr_sub(dev, truth, a.re, MPFR_RNDN);
    mpfr_abs(dev, dev, MPFR_RNDN);

    const bool encloses = mpfr_cmp_d(dev, a.rr) <= 0;
    ZF_CHECK(encloses);
    ZF_CHECK(mpfr_get_prec(a.re) == kWp);
    ZF_CHECK(mpfr_get_prec(a.im) == kWp);
    std::printf("C5_PRECISION_RULE encloses=%d dev=%.6g rr=%.6g "
                "stored_after=%d\n",
                static_cast<int>(encloses), mpfr_get_d(dev, MPFR_RNDN), a.rr,
                static_cast<int>(mpfr_get_prec(a.re)));

    mpfr_clear(cf);
    mpfr_clear(eps);
    mpfr_clear(truth);
    mpfr_clear(dev);
  }

  std::fprintf(stdout, "CBALL_SUITE failures %d\n", ::zftest::failure_count());
  return ::zftest::failure_count() == 0 ? 0 : 1;
}
