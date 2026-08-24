// Stage 4 Z(t) certification suite: ARMED, and red until the EM path lands.
//
// Decision note (readiness review findings C1, A1). The previous version of
// this file was green while zeta_em was an unimplemented throwing stub, and it
// stayed green when the rev 0 defect (return Ball 0.0 with ZStatus::Certified)
// was reinstated verbatim. It achieved that by catching the throw and
// continuing, under a comment claiming it verified the throw message, and by
// printing the count of combinations that did NOT skip under a label reading
// L1A_SKIPPED. Nothing in it asserted an enclosure.
//
// This suite is now pre-registered rather than decorative:
//   - It is NOT in the default ctest set. It is registered only when the build
//     is configured with -DZF_ARM_STAGE4=ON, so a green default suite never
//     implies stage 4 works.
//   - A throw from zeta_em is a FAILURE, not a skip. There is no catch that
//     swallows it. With the current stub this binary exits 1 by design: that
//     is the honest state of stage 4, and the gate battery records it.
//
// Oracle: acb_dirichlet_hardy_z, a rigorous interval evaluation of Hardy's Z
// from FLINT. It shares no code with the theta, coefficient or ball
// implementation. Enclosure is tested as NON-DISJOINTNESS with zero additive
// slack: both intervals claim to contain the same true Z(t), so disjoint
// intervals prove one of them unsound, and no ulp nudges are applied to hide
// a boundary case.
//
// Layers:
//   L-A ENCLOSURE  our certified ball must intersect the oracle interval over
//                  a sweep of heights x working precisions.
//   L-B GAMMA_1    the stage 4 definition-of-done target: a certified sign
//                  bracket around the first zero. Z(14) must certify NEGATIVE
//                  and Z(15) must certify POSITIVE (ball strictly one side of
//                  zero), and at gamma_1 = 14.134725141... the ball must
//                  either contain zero or report Contested. Review finding A1
//                  records why this cannot pass under MATHS.md D8 as written:
//                  |Z| = |zeta| carries no sign, so a certified NEGATIVE value
//                  is unreachable from a magnitude. The layer is left armed
//                  and failing rather than weakened to match the design.

#include <cstdio>
#include <cmath>
#include <exception>

#include "zetaforge/ball.hpp"
#include "zetaforge/em_eval.hpp"

#include <mpfr.h>
#include <flint/arb.h>
#include <flint/acb.h>
#include <flint/acb_dirichlet.h>

#include "check.hpp"

using zetaforge::Ball;
using zetaforge::ZResult;
using zetaforge::ZStatus;

namespace {

constexpr slong kOraclePrec = 256;

// Rigorous enclosure of Z(t) from FLINT, as [lo, hi] doubles rounded outward.
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

// Our claimed interval, as [centre - radius, centre + radius] rounded outward.
void ours(const Ball& b, double& lo, double& hi) {
  const double c = mpfr_get_d(b.centre(), MPFR_RNDN);
  const double r = b.radius();
  lo = c - r;
  hi = c + r;
}

}  // namespace

static int run_suite() {
  std::fprintf(stdout, "SEED %llx\n",
               static_cast<unsigned long long>(::zftest::current_seed()));

  // ---- L-A: enclosure sweep ----------------------------------------------
  {
    const double heights[] = {
        0.5, 1.0, 2.0, 5.0, 10.0, 14.0, 14.134725141, 15.0,
        20.0, 50.0, 100.0, 150.0, 199.9, 200.0, 250.0,
        500.0, 1000.0, 5000.0};
    const int n_heights = static_cast<int>(sizeof(heights) / sizeof(heights[0]));
    const mpfr_prec_t precs[] = {128, 256};

    int evaluated = 0;
    int enclosure_failures = 0;

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
          std::printf("LA_DISJOINT t=%.9g prec=%d ours=[%.17g,%.17g] "
                      "oracle=[%.17g,%.17g]\n",
                      h, static_cast<int>(precs[pi]), mlo, mhi, olo, ohi);
        }
        ZF_CHECK(intersects);
        ZF_CHECK(r.re.radius() > 0.0);
      }
    }
    std::fprintf(stdout, "LA_EVALUATED %d\n", evaluated);
    std::fprintf(stdout, "LA_ENCLOSURE_FAILURES %d\n", enclosure_failures);
  }

  // ---- L-B: gamma_1 certified sign bracket -------------------------------
  {
    const ZResult below = zetaforge::zeta_em(14.0, 256);
    const ZResult above = zetaforge::zeta_em(15.0, 256);
    const ZResult at = zetaforge::zeta_em(14.134725141, 256);

    double lo = 0.0, hi = 0.0;

    // Z(14) must be certified strictly negative.
    ours(below.re, lo, hi);
    const bool neg = hi < 0.0 && below.status == ZStatus::Certified;
    ZF_CHECK(neg);

    // Z(15) must be certified strictly positive.
    ours(above.re, lo, hi);
    const bool pos = lo > 0.0 && above.status == ZStatus::Certified;
    ZF_CHECK(pos);

    // At the zero itself the ball may not claim a definite sign.
    const bool straddles =
        at.re.contains_zero() || at.status == ZStatus::Contested;
    ZF_CHECK(straddles);

    std::fprintf(stdout, "LB_BRACKET neg=%d pos=%d straddles=%d\n",
                 static_cast<int>(neg), static_cast<int>(pos),
                 static_cast<int>(straddles));
  }

  std::fprintf(stdout, "ZETA_SUITE failures %d\n", ::zftest::failure_count());
  return ::zftest::failure_count() == 0 ? 0 : 1;
}

// An exception out of zeta_em is a stage 4 failure. This handler exists to
// turn it into a clean non-zero exit rather than a SIGABRT, and it does NOT
// resume the sweep: the suite stops at the first unmet obligation. It makes no
// claim about the exception's message, unlike the catch it replaces.
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
