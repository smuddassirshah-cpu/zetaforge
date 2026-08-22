#include "zetaforge/radius.hpp"
#include "zetaforge/theta.hpp"

#include <cmath>
#include <stdexcept>

namespace zetaforge {

namespace {

// Transcribed from tools/discover_theta_coefficients.py (oracle-verified).
// c7 doubles as the first omitted magnitude for the N=6 remainder bound;
// c8 refines it. Both are positive, so the first-omitted-term principle
// for this expansion bounds the truncation error by their magnitudes.
constexpr double kC[kThetaTerms] = {
    1.0 / 48.0,                 // exact 1/48
    7.0 / 5760.0,               // exact 7/5760
    31.0 / 80640.0,             // exact 31/80640
    127.0 / 430080.0,           // exact 127/430080
    511.0 / 1216512.0,          // exact 511/1216512
    1414477.0 / 1476034560.0,   // exact
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

}  // namespace

Ball theta_certified(double t, mpfr_prec_t prec) {
  if (!std::isfinite(t)) {
    throw std::invalid_argument("theta requires finite t");
  }
  if (t < kThetaTMin) {
    throw std::domain_error(
        "theta series below RS-validity threshold t0=200; EM path owns this range");
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
  MpfrGuard u(p), contrib(p), upow(p);
  mpfr_set_d(u.v, t, MPFR_RNDN);
  mpfr_sqr(u.v, u.v, MPFR_RNDN);
  mpfr_si_div(u.v, 1, u.v, MPFR_RNDN);
  mpfr_set(upow.v, u.v, MPFR_RNDN);
  for (int k = 1; k <= kThetaTerms; ++k) {
    mpfr_set_d(contrib.v, kC[k - 1], MPFR_RNDN);
    mpfr_mul(contrib.v, contrib.v, half_t.v, MPFR_RNDN);
    mpfr_mul(contrib.v, contrib.v, upow.v, MPFR_RNDN);
    mpfr_mul_2si(contrib.v, contrib.v, 1, MPFR_RNDN);  // factor 2 from (t/2) form
    mpfr_add(acc.v, acc.v, contrib.v, MPFR_RNDN);
    if (k < kThetaTerms) {
      mpfr_mul(upow.v, upow.v, u.v, MPFR_RNDN);
    }
  }

  // Certified radius: remainder bound times safety, plus mpfr rounding slack.
  const double t2 = t * t;
  const double rem_base = (kC7 + kC8 / t2) / std::pow(t, 13.0);
  const double mag_d = std::fabs(mpfr_get_d(acc.v, MPFR_RNDN));
  const double mpfr_slack = (mag_d > 0.0 ? mag_d : 1.0) * std::ldexp(1.0, static_cast<int>(-(prec - 2)));
  double radius = kThetaSafety * rem_base + mpfr_slack;
  radius = inflate(radius);

  return Ball::from_centre_and_radius(acc.v, radius);
}

}  // namespace zetaforge
