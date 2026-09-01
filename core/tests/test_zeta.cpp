// Stage 4 Z(t) certification suite.
//
// Decision note (readiness review findings C1, A1). Through rev 1 this file was
// green while zeta_em was an unimplemented throwing stub, and stayed green when
// the rev 0 defect (return Ball 0.0 with ZStatus::Certified) was reinstated
// verbatim. It achieved that by catching the throw and continuing, under a
// comment claiming it verified the throw message. Nothing in it asserted an
// enclosure. At rev 5 it was armed and left RED. It goes green at rev 6 because
// the EM path exists, not because anything here was weakened.
//
// A throw from zeta_em is a FAILURE, not a skip. There is no catch that
// swallows one.
//
// Oracle: acb_dirichlet_hardy_z, a rigorous interval evaluation of Hardy's Z
// from FLINT. It shares no code with the theta, coefficient, Bernoulli or ball
// implementation. Enclosure is tested as NON-DISJOINTNESS with zero additive
// slack: both intervals claim to contain the same true Z(t), so disjoint
// intervals prove one of them unsound, and no ulp nudges are applied to hide a
// boundary case.
//
// Layers:
//   L-A ENCLOSURE  our certified ball must intersect the oracle interval over
//                  a sweep of heights x working precisions.
//   L-B GAMMA_1    the stage 4 definition-of-done target. Z is certified
//                  NEGATIVE below gamma_1 = 14.134725141734694 and certified
//                  POSITIVE above it, so the sign change that defines the zero
//                  is isolated by two certified balls of definite and opposite
//                  sign. Stated explicitly because the direction is a claim:
//                  Z increases through this zero.
//   L-C IMAGINARY  Z(t) is real, so Im[e^{i theta} zeta] must be a ball
//                  containing zero (MATHS.md D8.9). A free simultaneous check
//                  on theta's branch, on the sign convention of the assembly
//                  and on the zeta ball.
//   L-D POLICY     the EM tail bound must equal an independent transcription
//                  of Backlund's bound, computed here from FLINT's Bernoulli
//                  numbers rather than ours. This is what detects a remainder
//                  policy that has been quietly weakened; enclosure cannot,
//                  because the bound has room over the true error by design.
//   DOMAIN         t <= 0, non-finite t, and t > kEmTMax are refused.

#include <cstdio>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <limits>

#include "check.hpp"
#include "zetaforge/ball.hpp"
#include "zetaforge/em_eval.hpp"

#include <mpfr.h>
#include <flint/arb.h>
#include <flint/acb.h>
#include <flint/acb_dirichlet.h>
#include <flint/fmpq.h>
#include <flint/bernoulli.h>

using zetaforge::Ball;
using zetaforge::ZResult;
using zetaforge::ZStatus;
using zetaforge::kEmTMax;

namespace {

constexpr slong kOraclePrec = 320;

// gamma_1 = 14.134725141734693790... The bracket points straddle it by about
// 5e-11, which is far outside every certified radius here and far inside the
// spacing to the next zero at 21.02.
constexpr double kGamma1Below = 14.1347251417;
constexpr double kGamma1Above = 14.1347251418;

void oracle_hardy_z(double t, double& lo, double& hi) {
  acb_t tt, z;
  arb_t re;
  acb_init(tt);
  acb_init(z);
  arb_init(re);
  acb_set_d(tt, t);
  acb_dirichlet_hardy_z(z, tt, nullptr, nullptr, 1, kOraclePrec);
  acb_get_real(re, z);
  arf_t a, b;
  arf_init(a);
  arf_init(b);
  arb_get_lbound_arf(a, re, 64);
  arb_get_ubound_arf(b, re, 64);
  lo = arf_get_d(a, ARF_RND_FLOOR);
  hi = arf_get_d(b, ARF_RND_CEIL);
  arf_clear(a);
  arf_clear(b);
  arb_clear(re);
  acb_clear(z);
  acb_clear(tt);
}

void ours(const Ball& b, double& lo, double& hi) {
  const double c = mpfr_get_d(b.centre(), MPFR_RNDN);
  const double r = b.radius();
  lo = c - r;
  hi = c + r;
}

// Backlund's bound (Edwards 6.4) transcribed independently of core/src:
//   |T_{M+1}| |s + 2M+1| / (sigma + 2M+1)
// with Bernoulli numbers from FLINT rather than from our recurrence, so a
// defect shared between the production bound and its own Bernoulli source
// cannot hide here.
double backlund_transcribed(double t, long n, int m) {
  const mpfr_prec_t p = 400;
  mpfr_t acc, tmp, term;
  mpfr_inits2(p, acc, tmp, term, static_cast<mpfr_ptr>(nullptr));

  fmpq_t bq;
  fmpq_init(bq);
  bernoulli_fmpq_ui(bq, static_cast<ulong>(2 * (m + 1)));
  mpq_t bm;
  mpq_init(bm);
  fmpq_get_mpq(bm, bq);
  mpfr_set_q(acc, bm, MPFR_RNDU);
  mpfr_abs(acc, acc, MPFR_RNDU);
  mpq_clear(bm);
  fmpq_clear(bq);

  mpfr_fac_ui(tmp, static_cast<unsigned long>(2 * (m + 1)), MPFR_RNDD);
  mpfr_div(acc, acc, tmp, MPFR_RNDU);

  for (long j = 0; j <= 2L * m; ++j) {
    mpfr_set_d(tmp, 0.5 + static_cast<double>(j), MPFR_RNDU);
    mpfr_sqr(tmp, tmp, MPFR_RNDU);
    mpfr_set_d(term, t, MPFR_RNDU);
    mpfr_sqr(term, term, MPFR_RNDU);
    mpfr_add(tmp, tmp, term, MPFR_RNDU);
    mpfr_sqrt(tmp, tmp, MPFR_RNDU);
    mpfr_mul(acc, acc, tmp, MPFR_RNDU);
  }

  mpfr_set_si(tmp, n, MPFR_RNDU);
  mpfr_pow_si(term, tmp, -(2L * m + 2L), MPFR_RNDU);
  mpfr_mul(acc, acc, term, MPFR_RNDU);
  mpfr_sqrt(term, tmp, MPFR_RNDU);
  mpfr_mul(acc, acc, term, MPFR_RNDU);

  const double shift = static_cast<double>(2 * m + 1);
  mpfr_set_d(tmp, 0.5 + shift, MPFR_RNDU);
  mpfr_sqr(tmp, tmp, MPFR_RNDU);
  mpfr_set_d(term, t, MPFR_RNDU);
  mpfr_sqr(term, term, MPFR_RNDU);
  mpfr_add(tmp, tmp, term, MPFR_RNDU);
  mpfr_sqrt(tmp, tmp, MPFR_RNDU);
  mpfr_mul(acc, acc, tmp, MPFR_RNDU);
  mpfr_set_d(tmp, 0.5 + shift, MPFR_RNDD);
  mpfr_div(acc, acc, tmp, MPFR_RNDU);

  const double out = mpfr_get_d(acc, MPFR_RNDU);
  mpfr_clears(acc, tmp, term, static_cast<mpfr_ptr>(nullptr));
  return out;
}

}  // namespace

static int run_suite() {
  std::fprintf(stdout, "SEED %llx\n",
               static_cast<unsigned long long>(::zftest::current_seed()));

  // ---- L-A, L-C, L-D over the sweep --------------------------------------
  {
    const double heights[] = {
        0.5, 1.0, 2.0, 5.0, 10.0, 14.0, kGamma1Below, kGamma1Above, 15.0,
        20.0, 21.022039639, 25.01085758, 50.0, 100.0, 150.0, 199.9, 200.0,
        250.0, 300.0, 400.0};
    const int n_heights = static_cast<int>(sizeof(heights) / sizeof(heights[0]));
    const mpfr_prec_t precs[] = {128, 256};

    int evaluated = 0, enclosure_failures = 0, im_failures = 0;
    int policy_failures = 0;
    double worst_policy = 0.0;

    for (int pi = 0; pi < 2; ++pi) {
      for (int hi_i = 0; hi_i < n_heights; ++hi_i) {
        const double h = heights[hi_i];
        // No try/catch: an exception here is a stage 4 failure, and the
        // process exiting non-zero on it is the correct signal.
        const ZResult r = zetaforge::zeta_em(h, precs[pi]);
        ++evaluated;

        double olo = 0.0, ohi = 0.0, mlo = 0.0, mhi = 0.0;
        oracle_hardy_z(h, olo, ohi);
        ours(r.re, mlo, mhi);
        const bool intersects = !(mhi < olo || ohi < mlo);
        if (!intersects) {
          ++enclosure_failures;
          std::printf("LA_DISJOINT t=%.17g prec=%d ours=[%.17g,%.17g] "
                      "oracle=[%.17g,%.17g]\n",
                      h, static_cast<int>(precs[pi]), mlo, mhi, olo, ohi);
        }
        ZF_CHECK(intersects);
        ZF_CHECK(r.re.radius() > 0.0);

        // L-C: Z is real, so the imaginary part must contain zero.
        if (!r.im.contains_zero()) {
          ++im_failures;
          std::printf("LC_IMAGINARY t=%.17g prec=%d im=%.6g rad=%.6g\n",
                      h, static_cast<int>(precs[pi]),
                      mpfr_get_d(r.im.centre(), MPFR_RNDN), r.im.radius());
        }
        ZF_CHECK(r.im.contains_zero());

        // L-D: the EM tail bound must be Backlund's, at the pinned (N, M).
        //
        // Band history (rev 7, B3). Through rev 6 this accepted any ratio in
        // (0.99, 1.01) while the measured production/transcription agreement
        // was ratio - 1 = 0.0 exactly, at every one of the 40 combos: both
        // sides are deterministic mpfr sequences landing on the same double.
        // A one percent band over an exact agreement meant NO transcription
        // drift below one percent could ever fail this layer, so it was not a
        // test of its own transcription. The band is now 1e-12: eight orders
        // tighter than row 26's pre-registered 1e-4 perturbation, four orders
        // looser than any benign last-bit divergence could reach, and honest
        // about what the layer actually measures.
        const double expect = backlund_transcribed(h, r.em_n, r.em_m);
        const double ratio = r.em_tail_bound / expect;
        if (ratio > worst_policy) worst_policy = ratio;
        const bool policy_ok = std::fabs(ratio - 1.0) <= 1e-12;
        if (!policy_ok) {
          ++policy_failures;
          std::printf("LD_POLICY t=%.17g N=%d M=%d ours=%.6g expect=%.6g "
                      "ratio=%.6g\n",
                      h, r.em_n, r.em_m, r.em_tail_bound, expect, ratio);
        }
        ZF_CHECK(policy_ok);
      }
    }
    ZF_CHECK(evaluated == 2 * n_heights);
    std::fprintf(stdout, "LA_EVALUATED %d\n", evaluated);
    std::fprintf(stdout, "LA_ENCLOSURE_FAILURES %d\n", enclosure_failures);
    std::fprintf(stdout, "LC_IMAGINARY_FAILURES %d\n", im_failures);
    std::fprintf(stdout, "LD_POLICY_FAILURES %d max_ratio %.17g\n",
                 policy_failures, worst_policy);
  }

  // ---- L-B: gamma_1 certified sign bracket -------------------------------
  // The stage 4 definition-of-done target. Z INCREASES through gamma_1: it is
  // certified negative below and certified positive above. The direction is
  // stated rather than inferred, so swapping the endpoints (ATTACKS.md row 20)
  // fails rather than passing by symmetry.
  {
    const ZResult below = zetaforge::zeta_em(kGamma1Below, 256);
    const ZResult above = zetaforge::zeta_em(kGamma1Above, 256);

    double lo = 0.0, hi = 0.0;
    ours(below.re, lo, hi);
    const bool neg = hi < 0.0 && below.status == ZStatus::Certified;
    ZF_CHECK(neg);
    const double below_hi = hi;

    ours(above.re, lo, hi);
    const bool pos = lo > 0.0 && above.status == ZStatus::Certified;
    ZF_CHECK(pos);
    const double above_lo = lo;

    // Both balls are strictly one side of zero and on OPPOSITE sides, so a
    // sign change is certified inside a bracket of width 1e-10.
    const bool isolated = neg && pos && below_hi < 0.0 && above_lo > 0.0;
    ZF_CHECK(isolated);

    std::fprintf(stdout,
                 "LB_BRACKET below=%.17g Zhi=%.6g above=%.17g Zlo=%.6g "
                 "neg=%d pos=%d isolated=%d width=%.3g\n",
                 kGamma1Below, below_hi, kGamma1Above, above_lo,
                 static_cast<int>(neg), static_cast<int>(pos),
                 static_cast<int>(isolated), kGamma1Above - kGamma1Below);
  }

  // ---- DOMAIN -------------------------------------------------------------
  {
    int rejected = 0;
    for (double bad : {0.0, -1.0}) {
      try {
        auto r = zetaforge::zeta_em(bad, 128);
        (void)r;
      } catch (const std::invalid_argument&) {
        ++rejected;
      }
    }
    try {
      auto r = zetaforge::zeta_em(std::numeric_limits<double>::quiet_NaN(), 128);
      (void)r;
    } catch (const std::invalid_argument&) {
      ++rejected;
    }
    // Above kEmTMax the RS path owns the range. Refusing is the contract, and
    // the heights the rev 1 sweep evaluated there are asserted to refuse
    // rather than dropped from the suite.
    for (double above : {kEmTMax + 1.0, 500.0, 1000.0, 5000.0, 20000.0}) {
      try {
        auto r = zetaforge::zeta_em(above, 128);
        (void)r;
      } catch (const std::domain_error&) {
        ++rejected;
      }
    }
    ZF_CHECK(rejected == 8);
    std::fprintf(stdout, "ZETA_DOMAIN_REJECTED %d\n", rejected);
  }

  std::fprintf(stdout, "ZETA_SUITE failures %d\n", ::zftest::failure_count());
  return ::zftest::failure_count() == 0 ? 0 : 1;
}

int main() {
  try {
    return run_suite();
  } catch (const std::exception& e) {
    std::fprintf(stderr,
                 "ZF_CHECK failed: zeta_em threw (%s) [seed %llx]\n",
                 e.what(),
                 static_cast<unsigned long long>(::zftest::current_seed()));
    std::fprintf(stdout, "ZETA_SUITE aborted by exception; failures %d\n",
                 ::zftest::failure_count() + 1);
    return 1;
  }
}
