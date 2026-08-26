#include "zetaforge/bernoulli.hpp"
#include "zetaforge/radius.hpp"
#include "zetaforge/sabotage.hpp"
#include "zetaforge/theta.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace zetaforge {

namespace {

// Exact rational coefficients (MATHS.md D1). The provenance claim that stood
// here (a global Newton polish against mpmath loggamma matching the oracle to
// 4e-62) was retracted at stage 3 rev 1 as a degenerate overfit and is struck
// here too (readiness review finding A4). The values below are correct and
// were re-verified independently against the classical closed form for the
// theta asymptotic coefficients; the committed discovery script does NOT
// currently regenerate them, which is an open provenance defect owned by
// Phase 2, not a defect in this table. Parsed at working precision;
// per-coefficient representation error <= ulp_p(c_k) is carried explicitly
// in the certified radius (coeff_slack), never silently dropped.
struct RationalCoeff { unsigned long long num; unsigned long long den; };
constexpr RationalCoeff kCRat[kThetaTerms] = {
    {1ULL, 48ULL},
    {7ULL, 5760ULL},
    {31ULL, 80640ULL},
    {127ULL, 430080ULL},
    {511ULL, 1216512ULL},
    {1414477ULL, 1476034560ULL},   // per MATHS.md D1
};
constexpr double kC7 = 8191.0 / 2555904.0;      // first omitted magnitude
constexpr double kC8 = 0.014774875890195759;    // next magnitude estimate


struct MpfrGuard {
  mpfr_t v;
  explicit MpfrGuard(mpfr_prec_t p) { mpfr_init2(v, p); }
  ~MpfrGuard() { mpfr_clear(v); }
  MpfrGuard(const MpfrGuard&) = delete;
  MpfrGuard& operator=(const MpfrGuard&) = delete;
};


// ---- sub-t0 path (MATHS.md D8) ------------------------------------------
//
// theta(t) = Im logGamma(1/4 + it/2) - (t/2) ln pi, for 0 < t < t0.
//
// logGamma(z) = logGamma(z + m) - sum_{j<m} Log(z + j), principal logs, with m
// chosen so that w = z + m sits in the sector |arg w| <= pi/4 where the
// Stieltjes remainder constant is validated, and far enough out that the
// Stirling series reaches the target before it diverges.

struct MpqGuard {
  mpq_t v;
  MpqGuard() { mpq_init(v); }
  ~MpqGuard() { mpq_clear(v); }
  MpqGuard(const MpqGuard&) = delete;
  MpqGuard& operator=(const MpqGuard&) = delete;
};

// c_n = B_{2n} / (2n(2n-1)), exact rational evaluated at precision p.
void stirling_coeff(unsigned n, mpfr_ptr out, mpfr_prec_t p) {
  MpqGuard q;
  bernoulli_2n(n, q.v);
  mpq_t denom;
  mpq_init(denom);
  mpq_set_ui(denom, 2u * n * (2u * n - 1u), 1u);
  mpq_div(q.v, q.v, denom);
  mpq_clear(denom);
  mpfr_set_q(out, q.v, MPFR_RNDN);
  (void)p;
}

// The m rule. Two independent requirements, both load-bearing:
//   (a) sector: Re w >= Im w keeps |arg w| <= pi/4, where the Stieltjes
//       constant sec^(2N+2)(arg w / 2) is validated. m >= t/2 gives
//       Re w = 1/4 + m > t/2 = Im w.
//   (b) decay: the Stirling remainder bottoms out near e^(-2 pi |w|), and the
//       sec factor costs back e^(2 pi |w| ln sec(pi/8)), leaving a net
//       e^(-5.7853 |w|). Reaching 2^-q needs |w| >= q ln2 / 5.7853 = 0.1198 q.
//       0.14 q plus two spare steps carries the polynomial factor.
unsigned long theta_shift(double t, long q) {
  const double sector = std::ceil(t / 2.0);
  const double decay = std::ceil(0.14 * static_cast<double>(q)) + 2.0;
  return static_cast<unsigned long>(std::max(sector, decay));
}

Ball theta_subt0(double t, mpfr_prec_t prec) {
  const mpfr_prec_t P = prec + 64;
  const long q = static_cast<long>(prec) + 16;      // target truncation bits
  const unsigned long m = theta_shift(t, q);

  const double w_re = 0.25 + static_cast<double>(m);
  const double w_im = t / 2.0;

  MpfrGuard wr(P), wi(P), absw2(P), lnabsw(P), argw(P), acc(P), tmp(P), tmp2(P);
  mpfr_set_d(wr.v, w_re, MPFR_RNDN);
  mpfr_set_d(wi.v, w_im, MPFR_RNDN);
  mpfr_sqr(absw2.v, wr.v, MPFR_RNDN);
  mpfr_sqr(tmp.v, wi.v, MPFR_RNDN);
  mpfr_add(absw2.v, absw2.v, tmp.v, MPFR_RNDN);
  mpfr_log(lnabsw.v, absw2.v, MPFR_RNDN);
  mpfr_div_2ui(lnabsw.v, lnabsw.v, 1, MPFR_RNDN);   // ln|w|
  mpfr_atan2(argw.v, wi.v, wr.v, MPFR_RNDN);

  // Sector guard. The remainder constant below is only validated for
  // |arg w| <= pi/4; outside it the bound is not a bound. Checked rather than
  // assumed (docs/gate/ATTACKS.md row 21).
  if (!(w_im <= w_re)) {
    throw std::logic_error(
        "theta sub-t0: shift left w outside the validated Stirling sector");
  }

  // Im[(w - 1/2) Log w] - Im w
  mpfr_sub_d(tmp.v, wr.v, 0.5, MPFR_RNDN);
  mpfr_mul(acc.v, tmp.v, argw.v, MPFR_RNDN);        // (Re w - 1/2) arg w
  mpfr_mul(tmp.v, wi.v, lnabsw.v, MPFR_RNDN);       // Im w ln|w|
  mpfr_add(acc.v, acc.v, tmp.v, MPFR_RNDN);
  mpfr_sub(acc.v, acc.v, wi.v, MPFR_RNDN);          // - Im w
  // + (1/2) ln(2 pi) is real: it contributes nothing to the imaginary part.

  // Stirling series in x = 1/w. Terms are c_n x^(2n-1); track p = x^(2n-1).
  MpfrGuard xr(P), xi(P), x2r(P), x2i(P), pr(P), pi_(P), cn(P);
  mpfr_div(xr.v, wr.v, absw2.v, MPFR_RNDN);
  mpfr_div(xi.v, wi.v, absw2.v, MPFR_RNDN);
  mpfr_neg(xi.v, xi.v, MPFR_RNDN);                  // x = conj(w)/|w|^2
  mpfr_sqr(x2r.v, xr.v, MPFR_RNDN);
  mpfr_sqr(tmp.v, xi.v, MPFR_RNDN);
  mpfr_sub(x2r.v, x2r.v, tmp.v, MPFR_RNDN);
  mpfr_mul(x2i.v, xr.v, xi.v, MPFR_RNDN);
  mpfr_mul_2ui(x2i.v, x2i.v, 1, MPFR_RNDN);         // x^2
  mpfr_set(pr.v, xr.v, MPFR_RNDN);
  mpfr_set(pi_.v, xi.v, MPFR_RNDN);

  // Stieltjes sectoral remainder: |r_N(w)| <= |c_(N+1)| |w|^-(2N+1)
  // sec^(2N+2)(arg w / 2). Tracked as a double, rounded outward.
  MpfrGuard sec_half(P), invabsw(P);
  mpfr_div_2ui(tmp.v, argw.v, 1, MPFR_RNDN);
  mpfr_cos(tmp.v, tmp.v, MPFR_RNDN);
  mpfr_ui_div(sec_half.v, 1, tmp.v, MPFR_RNDN);
  mpfr_sqrt(tmp.v, absw2.v, MPFR_RNDN);
  mpfr_ui_div(invabsw.v, 1, tmp.v, MPFR_RNDN);      // 1/|w|

  const double inv_absw = mpfr_get_d(invabsw.v, MPFR_RNDU);
  const double sec2 = std::pow(mpfr_get_d(sec_half.v, MPFR_RNDU), 2.0);
  const double target = std::ldexp(1.0, static_cast<int>(-q));

  auto bound_after = [&](unsigned n) {
    // |c_(n+1)| |w|^-(2n+1) sec^(2n+2)(arg w / 2), outward at every step.
    stirling_coeff(n + 1, cn.v, P);
    mpfr_abs(tmp.v, cn.v, MPFR_RNDU);
    double b = mpfr_get_d(tmp.v, MPFR_RNDU);
    for (unsigned k = 0; k < 2 * n + 1; ++k) {
      b = up_mul(b, inv_absw);
    }
    for (unsigned k = 0; k < n + 1; ++k) {
      b = up_mul(b, sec2);
    }
    return b;
  };

  unsigned N = 0;
  double trunc = bound_after(0);
  while (trunc > target && N + 1 < kBernoulliMaxN) {
    ++N;
    stirling_coeff(N, cn.v, P);
    mpfr_mul(tmp.v, cn.v, pi_.v, MPFR_RNDN);        // Im(c_N x^(2N-1))
    mpfr_add(acc.v, acc.v, tmp.v, MPFR_RNDN);
    // p *= x^2
    mpfr_mul(tmp.v, pr.v, x2r.v, MPFR_RNDN);
    mpfr_mul(tmp2.v, pi_.v, x2i.v, MPFR_RNDN);
    mpfr_sub(tmp.v, tmp.v, tmp2.v, MPFR_RNDN);
    mpfr_mul(tmp2.v, pr.v, x2i.v, MPFR_RNDN);
    mpfr_mul(pi_.v, pi_.v, x2r.v, MPFR_RNDN);
    mpfr_add(pi_.v, pi_.v, tmp2.v, MPFR_RNDN);
    mpfr_set(pr.v, tmp.v, MPFR_RNDN);
    const double next = bound_after(N);
    if (next > trunc) {
      break;   // divergence turnover: no later N is tighter
    }
    trunc = next;
  }

  // Recurrence back down: subtract arg(z + j) for j = 0 .. m-1. Every z + j
  // has positive real part, so atan2 returns the principal argument and the
  // continuous branch coincides with it (D8 clause (a)).
  MpfrGuard zj(P), half_t(P);
  mpfr_set_d(half_t.v, w_im, MPFR_RNDN);
  for (unsigned long j = 0; j < m; ++j) {
    mpfr_set_d(zj.v, 0.25 + static_cast<double>(j), MPFR_RNDN);
    mpfr_atan2(tmp.v, half_t.v, zj.v, MPFR_RNDN);
    mpfr_sub(acc.v, acc.v, tmp.v, MPFR_RNDN);
  }

  // - (t/2) ln pi
  MpfrGuard lpi(P);
  mpfr_const_pi(lpi.v, MPFR_RNDN);
  mpfr_log(lpi.v, lpi.v, MPFR_RNDN);
  mpfr_mul(tmp.v, half_t.v, lpi.v, MPFR_RNDN);
  mpfr_sub(acc.v, acc.v, tmp.v, MPFR_RNDN);

  // Rounding budget: every mpfr operation at precision P commits at most
  // |result| 2^-P, and no intermediate here exceeds the closed-form cap below.
  // Counted, not estimated: 12 fixed operations, 6 per series term, 3 per
  // recurrence step.
  const double mag_cap =
      2.0 * (static_cast<double>(m) + 1.0) * 1.5707963267948966 +
      w_im * (std::log(w_re + w_im) + 1.2) + 1.0;
  const double ops = 12.0 + 6.0 * static_cast<double>(N)
                     + 3.0 * static_cast<double>(m);
  const double unit = std::ldexp(1.0, 1 - static_cast<int>(P));
  const double round_slack = up_mul(up_mul(mag_cap, ops), unit);

  double radius = up_add(trunc, round_slack);
  radius *= zf_radius_sabotage_scale();
  radius = inflate(radius);
  return Ball::from_centre_and_radius(acc.v, radius);
}

}  // namespace

Ball theta_certified_loggamma(double t, mpfr_prec_t prec) {
  if (!std::isfinite(t)) {
    throw std::invalid_argument("theta requires finite t");
  }
  if (t <= 0.0) {
    throw std::domain_error("theta requires t > 0");
  }
  return theta_subt0(t, prec);
}

Ball theta_certified(double t, mpfr_prec_t prec) {
  if (!std::isfinite(t)) {
    throw std::invalid_argument("theta requires finite t");
  }
  if (t <= 0.0) {
    throw std::domain_error("theta requires t > 0");
  }
  if (t < kThetaTMin) {
    // Below t0 the asymptotic series has no certified bound. The log Gamma
    // path of MATHS.md D8 owns this range and returns a certified ball; the
    // throw that stood here through rev 5 is retired because a replacement
    // exists, not because the range got easier.
    return theta_subt0(t, prec);
  }

  const mpfr_prec_t p = prec + 32;  // guard bits: intermediate products only

  MpfrGuard half_t(p);
  mpfr_set_d(half_t.v, t, MPFR_RNDN);
  mpfr_div_2si(half_t.v, half_t.v, 1, MPFR_RNDN);   // t/2

  MpfrGuard tval(p), pi_v(p), lpi(p), l2(p), l2pi(p);
  mpfr_set_d(tval.v, t, MPFR_RNDN);
  mpfr_const_pi(pi_v.v, MPFR_RNDN);
  mpfr_log(lpi.v, pi_v.v, MPFR_RNDN);                // ln(pi)
  mpfr_const_log2(l2.v, MPFR_RNDN);                  // ln(2)
  mpfr_add(l2pi.v, l2.v, lpi.v, MPFR_RNDN);          // ln(2*pi)

  MpfrGuard acc(p);
  mpfr_log(acc.v, tval.v, MPFR_RNDN);                // ln(t)
  mpfr_sub(acc.v, acc.v, l2pi.v, MPFR_RNDN);         // ln(t/2pi)
  mpfr_mul(acc.v, acc.v, half_t.v, MPFR_RNDN);       // (t/2) ln(t/2pi)
  mpfr_sub(acc.v, acc.v, half_t.v, MPFR_RNDN);       // - t/2

  MpfrGuard pi8(p);
  mpfr_const_pi(pi8.v, MPFR_RNDN);
  mpfr_div_2si(pi8.v, pi8.v, 3, MPFR_RNDN);
  mpfr_sub(acc.v, acc.v, pi8.v, MPFR_RNDN);          // - pi/8

  // Series in u = 1/t^2: contribution_k = c_k / t^(2k-1) = 2*c_k*(t/2)*u^k.
  MpfrGuard u(p), contrib(p), upow(p), cf_num(p), cf_den(p);
  mpfr_set_d(u.v, t, MPFR_RNDN);
  mpfr_sqr(u.v, u.v, MPFR_RNDN);
  mpfr_si_div(u.v, 1, u.v, MPFR_RNDN);
  mpfr_set(upow.v, u.v, MPFR_RNDN);
  for (int k = 1; k <= kThetaTerms; ++k) {
    mpfr_set_uj(cf_num.v, kCRat[k - 1].num, MPFR_RNDN);
    mpfr_set_uj(cf_den.v, kCRat[k - 1].den, MPFR_RNDN);
    mpfr_div(contrib.v, cf_num.v, cf_den.v, MPFR_RNDN);
    mpfr_mul(contrib.v, contrib.v, half_t.v, MPFR_RNDN);
    mpfr_mul(contrib.v, contrib.v, upow.v, MPFR_RNDN);
    mpfr_mul_2si(contrib.v, contrib.v, 1, MPFR_RNDN);  // factor 2 from (t/2) form
    mpfr_add(acc.v, acc.v, contrib.v, MPFR_RNDN);
    if (k < kThetaTerms) {
      mpfr_mul(upow.v, upow.v, u.v, MPFR_RNDN);
    }
  }

  // Certified radius: three explicit components, each derived in MATHS.md D1:
  //   (i)   series remainder: SAFETY * (|c7| + |c8|/t^2) / t^13
  //   (ii)  mpfr rounding of the whole evaluation at working precision
  //   (iii) coefficient representation error from parsing rationals at p
  const double t2 = t * t;
  const double rem_base = (kC7 + kC8 / t2) / std::pow(t, 13.0);
  const double mag_d = std::fabs(mpfr_get_d(acc.v, MPFR_RNDN));
  const double mpfr_slack = (mag_d > 0.0 ? mag_d : 1.0)
                            * std::ldexp(1.0, static_cast<int>(-(prec - 2)));
  // (iii) coefficient representation error: each rational parsed at p
  // carries <= c_k * 2^(1-p); dominant term is c_1/t.
  const double coeff_slack = std::ldexp(1.0, static_cast<int>(1 - prec))
                             * ((1.0 / 48.0) / t) * (1.0 + 1e-3);
  double radius = kThetaSafety * rem_base + mpfr_slack + coeff_slack;
  // Compile-time gated falsifiability hook; an inline 1.0 in production
  // builds (zetaforge/sabotage.hpp). Never reads the environment here.
  radius *= zf_radius_sabotage_scale();
  radius = inflate(radius);

  return Ball::from_centre_and_radius(acc.v, radius);
}

}  // namespace zetaforge
