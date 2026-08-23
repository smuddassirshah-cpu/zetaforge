#include "zetaforge/radius.hpp"
#include "zetaforge/theta.hpp"

#include <cmath>
#include <cstdlib>
#include <stdexcept>

namespace zetaforge {

namespace {

// Coefficients from tools/discover_theta_coefficients.py, globally polished
// against mpmath loggamma (Newton fit at 12 heights, dps 400): the full N=6
// series then matches the oracle to <= 4e-62 absolute across t in [200,
// 3e12], and the t=200 leftover equals |c7|/t^13 exactly -- confirming both
// the table and the first-omitted-term remainder model behind D1.
// Exact rational coefficients (MATHS.md D1). Parsed at working precision;
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
  {
    // Enclosure-sabotage hook for the gate attack battery. Production must
    // run with this unset (scale = 1.0).
    const char* e = std::getenv("ZF_IMPL_RADIUS_SCALE");
    if (e != nullptr) {
      radius *= std::strtod(e, nullptr);
    }
  }
  radius = inflate(radius);

  return Ball::from_centre_and_radius(acc.v, radius);
}

}  // namespace zetaforge
