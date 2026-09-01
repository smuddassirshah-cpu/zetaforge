// Theta certification suite. Covers MATHS.md D1, D2 and D8.
//
// An earlier revision of this header and of DECISIONS.md described an
// "L1 ENCLOSURE" layer against FLINT acb_lgamma intervals over 84
// combinations. That layer has never existed in any committed tree; the claim
// is struck (readiness review finding D4). Nothing below may describe a layer
// this file does not execute.
//
//   DOMAIN  t0 is a dispatch switch, not a domain floor: below it the log
//       Gamma path returns a certified ball. The floor asserted here is the
//       real one, t > 0 and finite.
//   L2  GOLDEN, series path: committed mpmath values compared in full mpfr
//       precision; tolerance is exactly the certified radius (no additive
//       fudge). Corpus quantisation (~170 significant digits) sits far below
//       every certified radius, so the comparison is not corpus-limited.
//   L2b GOLDEN, log Gamma path (D8): same discipline, 28 heights from 1e-6 to
//       199.999 plus the overlap band. This layer is also what pins the
//       branch: a slip moves theta by a multiple of 2 pi, which no radius
//       here can absorb.
//   L4  OVERLAP: on [t0, 2 t0] both derivations are defined and must agree
//       within their combined radii. They share no coefficient, no truncation
//       argument and no remainder bound, so this is the only layer that
//       checks a certified radius against something that is neither a corpus
//       nor a transcription of its own derivation.
//   L5  BERNOULLI ORACLE: the exact recurrence that feeds both certified
//       series, against FLINT's own table (ATTACKS.md row 3).
//   L6  SECTOR: the Gamma-recurrence shift must keep |arg w| <= pi/4. A
//       tightness and validated-range invariant, not a soundness one
//       (ATTACKS.md row 21).
//   L3  POLICY EQUALITY: radius equals an independent transcription of the
//       full series-path derivation, within 1e-3 relative.
//
// Known limit, revised at rev 7 (B2): the D1 truncation bound is PROVEN
// (MATHS.md D1b; O1 closed) and 4x tighter than the retired factor-4 policy,
// so a halved truncation term now breaks L2 ENCLOSURE at the precisions where
// truncation dominates the radius, as well as L3 equality everywhere. L2
// still cannot see a too-small radius where the mpfr slack term dominates
// (low precisions); L3 covers that regime. The D8 path carries no safety
// factor either, so L2b and L4 bound it directly: as of rev 7 NO safety
// factor exists anywhere in theta.
//
// Sabotage hooks (compile-time gated: ZF_SABOTAGE_HOOKS, see core/src/sabotage.cpp):
//   ZF_IMPL_RADIUS_SCALE  scales production radius -> breaks L2 and L3
//   ZF_TEST_BOUND_SCALE   scales transcribed bound  -> breaks L3 equality
//   ZF_GOLDEN_PATH        points at corrupted corpus -> breaks L2
//   ZF_GOLDEN_SUBT0_PATH  points at corrupted sub-t0 corpus -> breaks L2b

#include <cmath>
#include <exception>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

// zetaforge headers first: they pull in gmp.h and mpfr.h, and FLINT guards its
// mpq/mpfr interoperability declarations on those having been seen already.
#include "check.hpp"
#include "zetaforge/ball.hpp"
#include "zetaforge/bernoulli.hpp"
#include "zetaforge/theta.hpp"

#include <flint/fmpq.h>
#include <flint/bernoulli.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef THETA_GOLDEN_CSV
#define THETA_GOLDEN_CSV "docs/golden/theta_golden.csv"
#endif

#ifndef THETA_GOLDEN_SUBT0_CSV
#define THETA_GOLDEN_SUBT0_CSV "docs/golden/theta_golden_subt0.csv"
#endif

using zetaforge::Ball;
using zetaforge::theta_certified;
using zetaforge::theta_certified_loggamma;

namespace {

constexpr mpfr_prec_t kPrec = 128;

const char* golden_path() {
  const char* e = std::getenv("ZF_GOLDEN_PATH");
  return e ? e : THETA_GOLDEN_CSV;
}

const char* golden_subt0_path() {
  const char* e = std::getenv("ZF_GOLDEN_SUBT0_PATH");
  return e ? e : THETA_GOLDEN_SUBT0_CSV;
}

double g_scale() {
  const char* e = std::getenv("ZF_TEST_BOUND_SCALE");
  return e ? std::strtod(e, nullptr) : 1.0;
}

double bound_base(double t) {
  // The PROVEN D1b bound, transcribed: c7/t^13 + (|B16|/15)/t^15 + e-term,
  // times the 1 + 2^-40 evaluation guard of D1b.7.
  const double c7 = 8191.0 / 2555904.0;
  const double tail_b16 = 0.4728105;   // > 3617/7650, as in production
  const double p13 = std::pow(t, 13.0);
  const double e4 = 4.0 * t;
  const double eterm =
      e4 >= 1074.0 ? std::numeric_limits<double>::denorm_min()
                   : std::ldexp(1.0, -static_cast<int>(e4));
  return (c7 / p13 + tail_b16 / (p13 * t * t) + eterm) * (1.0 + 0x1p-40);
}

// Independent transcription of ALL radius components for L3 policy
// equality. Deliberate duplication of theta.cpp's derivation constants:
// a mismatch means production changed without the suite agreeing.
double radius_transcribed(double t, int prec) {
  const double rem = bound_base(t);
  const double centre_scale =
      std::fabs((t / 2) * std::log(t / (2 * M_PI)) - t / 2 - M_PI / 8);
  const double mpfr_slack = centre_scale * std::ldexp(1.0, -(prec - 2));
  const double coeff_slack = std::ldexp(1.0, 1 - prec)
                             * ((1.0 / 48.0) / t) * (1.0 + 1e-3);
  return g_scale() * rem + mpfr_slack + coeff_slack;
}


// Our claimed interval shrunk inward by directed rounding must overlap the
// rigorous oracle interval. Zero additive slack: no ulp nudges anywhere.
}  // namespace

static int run_suite() {
  std::fprintf(stdout, "SEED %llx\n",
               static_cast<unsigned long long>(::zftest::current_seed()));

  // Domain. t0 is a dispatch switch, not a domain floor: 199.999 now returns a
  // certified ball from the log Gamma path (MATHS.md D8), so the assertion
  // that it THROWS is retired. It is replaced, not deleted: the real domain
  // floor is asserted instead, because a function that accepts t <= 0 or a NaN
  // would be a worse defect than the one the old assertion guarded.
  {
    const Ball below_t0 = theta_certified(199.999, kPrec);
    ZF_CHECK(below_t0.radius() > 0.0);
    ZF_CHECK(!below_t0.unknown_at_precision());

    int rejected = 0;
    for (double bad : {0.0, -1.0, -1e-300}) {
      try {
        auto b = theta_certified(bad, kPrec);
        (void)b;
      } catch (const std::domain_error&) {
        ++rejected;
      }
    }
    try {
      auto b = theta_certified(std::numeric_limits<double>::quiet_NaN(), kPrec);
      (void)b;
    } catch (const std::invalid_argument&) {
      ++rejected;
    }
    try {
      auto b = theta_certified(std::numeric_limits<double>::infinity(), kPrec);
      (void)b;
    } catch (const std::invalid_argument&) {
      ++rejected;
    }
    ZF_CHECK(rejected == 5);
    std::fprintf(stdout, "THETA_DOMAIN_REJECTED %d\n", rejected);
  }

  // ---- L1+L2: enclosure sweep against the committed corpus -------------
  // The oracle is mpmath loggamma (independent derivation path), committed as
  // the golden corpus. FLINT's acb_lgamma is NOT used here, and not because it
  // is defective: the reported defect was retracted as a harness artefact
  // (MATHS.md O2). No live oracle layer exists in this suite; see the header.
  {
    std::ifstream fh(golden_path());
    ZF_CHECK(fh.good());
    std::string line;
    int combos = 0;
    double tight_max = 0.0;
    double tight_min_margin = 1.0;
    mpfr_t g, d, tol;
    mpfr_init2(g, 1200); mpfr_init2(d, 1200); mpfr_init2(tol, 160);
    const mpfr_prec_t precs[4] = {128, 192, 256, 512};
    while (std::getline(fh, line)) {
      if (line.empty() || line[0] == '#') continue;
      if (line.rfind("t,value", 0) == 0) continue;
      const auto comma = line.find(',');
      if (comma == std::string::npos) continue;
      const double t = std::strtod(line.substr(0, comma).c_str(), nullptr);
      const std::string vstr = line.substr(comma + 1);
      if (mpfr_set_str(g, vstr.c_str(), 10, MPFR_RNDN) != 0) continue;
      for (int pi_idx = 0; pi_idx < 4; ++pi_idx) {
        const mpfr_prec_t pr = precs[pi_idx];
        const Ball th = theta_certified(t, pr);
        ZF_CHECK(th.radius() > 0.0);
        mpfr_sub(d, th.centre(), g, MPFR_RNDN);
        mpfr_abs(d, d, MPFR_RNDN);
        // Tolerance = the certified radius alone (zero additive slack).
        // The corpus carries 45 significant digits per value, so its own
        // rounding contribution sits far below any certified radius here.
        mpfr_set_d(tol, th.radius(), MPFR_RNDN);
        if (mpfr_cmp(d, tol) > 0) {
          std::printf("L2MISS pr=%d t=%g d=%.6g tol=%.6g ratio=%.3g\n",
                      (int)pr, t,
                      mpfr_get_d(d, MPFR_RNDN), mpfr_get_d(tol, MPFR_RNDN),
                      mpfr_get_d(d, MPFR_RNDN)/mpfr_get_d(tol, MPFR_RNDN));
        }
        ZF_CHECK(mpfr_cmp(d, tol) <= 0);

        // Regime bookkeeping: where the series bound dominates the mpfr
        // rounding term, the D1 claim itself is empirically testable. The
        // ratio is against the FULL radius (rev 7, B2): the measured error
        // includes the actual centre rounding, which only the slack term
        // absorbs, so err/radius is provably below 1 while err/trunc-bound
        // alone can graze it from either side in the far tight zone.
        const double mag = std::fabs(mpfr_get_d(th.centre(), MPFR_RNDN));
        const double slack = (mag > 0.0 ? mag : 1.0)
                           * std::ldexp(1.0, -(pr - 2));
        const double b = bound_base(t) * g_scale();
        if (slack <= b) {
          const double dd = mpfr_get_d(d, MPFR_RNDN);
          const double rat = dd / th.radius();
          if (rat > tight_max) tight_max = rat;
          // Exact relative margin (tol - d)/tol in mpfr: the double ratio
          // above saturates at 1.0 once the margin drops below double eps,
          // which it does for t >= ~1e9 where the D1b margin is ~143/t^2.
          mpfr_t mg;
          mpfr_init2(mg, 160);
          mpfr_sub(mg, tol, d, MPFR_RNDD);
          mpfr_div(mg, mg, tol, MPFR_RNDD);
          const double m = mpfr_get_d(mg, MPFR_RNDD);
          mpfr_clear(mg);
          if (m < tight_min_margin) tight_min_margin = m;
        }
        ++combos;
      }
    }
    mpfr_clear(g); mpfr_clear(d); mpfr_clear(tol);
    // Threshold is the exact corpus extent (21 heights x 4 precisions).
    // At 80 a single silently dropped or unparseable line passed green,
    // because the parse loop skips malformed rows (review finding C5).
    ZF_CHECK(combos >= 84);
    std::fprintf(stdout, "L12_COMBOS %d\n", combos);
    // Rev 7, B2. Under the proven D1b bound the true error approaches the
    // radius from BELOW with relative margin ~143/t^2, which falls under
    // double eps around t ~ 1e9: the double ratio then reads exactly 1.0
    // while the EXACT mpfr comparison above still holds with room. Both
    // numbers are printed so the evidence carries the margin, not just the
    // saturated ratio. The margin must stay strictly positive.
    std::fprintf(stdout, "L12_TIGHT_ZONE_MAX_ERR_OVER_RADIUS %.12f\n", tight_max);
    // 1e-13 floor: the 2^-40 evaluation guard (~9.1e-13) minus worst-case
    // accumulated double rounding (~1.5e-15, one-ulp libm pow included)
    // cannot fall below it on any conforming platform.
    ZF_CHECK(tight_min_margin > 1e-13);
    std::fprintf(stdout, "L12_TIGHT_ZONE_MIN_MARGIN %.6e\n", tight_min_margin);
  }

  // ---- L2b: sub-t0 corpus enclosure (MATHS.md D8) -----------------------
  // Same discipline as L2: tolerance is the certified radius exactly, with no
  // additive slack, compared in full mpfr precision. The corpus heights are
  // exact double expansions, because a reference taken at the decimal literal
  // instead of at the double it parses to differs by theta'(t) * 1e-16, which
  // is far above the certified radius.
  //
  // This layer is also what pins the branch. The recurrence and the Stirling
  // expansion are both on the principal branch; a slip on any factor moves
  // theta by a multiple of 2 pi, which no radius here can absorb.
  {
    std::ifstream fh(golden_subt0_path());
    ZF_CHECK(fh.good());
    std::string line;
    int combos = 0;
    double worst = 0.0;
    mpfr_t g, d, tol;
    mpfr_init2(g, 1200); mpfr_init2(d, 1200); mpfr_init2(tol, 160);
    const mpfr_prec_t precs[4] = {128, 192, 256, 512};
    while (std::getline(fh, line)) {
      if (line.empty() || line[0] == '#') continue;
      if (line.rfind("t,value", 0) == 0) continue;
      const auto comma = line.find(',');
      if (comma == std::string::npos) continue;
      const double t = std::strtod(line.substr(0, comma).c_str(), nullptr);
      if (mpfr_set_str(g, line.substr(comma + 1).c_str(), 10, MPFR_RNDN) != 0) {
        continue;
      }
      for (int pi_idx = 0; pi_idx < 4; ++pi_idx) {
        const mpfr_prec_t pr = precs[pi_idx];
        const Ball th = theta_certified_loggamma(t, pr);
        ZF_CHECK(th.radius() > 0.0);
        mpfr_sub(d, th.centre(), g, MPFR_RNDN);
        mpfr_abs(d, d, MPFR_RNDN);
        mpfr_set_d(tol, th.radius(), MPFR_RNDN);
        if (mpfr_cmp(d, tol) > 0) {
          std::printf("SUBT0MISS pr=%d t=%.17g d=%.6g tol=%.6g\n",
                      (int)pr, t, mpfr_get_d(d, MPFR_RNDN),
                      mpfr_get_d(tol, MPFR_RNDN));
        }
        ZF_CHECK(mpfr_cmp(d, tol) <= 0);
        const double ratio = mpfr_get_d(d, MPFR_RNDU) / th.radius();
        if (ratio > worst) worst = ratio;
        ++combos;
      }
    }
    mpfr_clear(g); mpfr_clear(d); mpfr_clear(tol);
    // 28 heights x 4 precisions. An exact threshold, not a lower bound: a
    // silently dropped or unparseable row must fail (review finding C5).
    ZF_CHECK(combos == 112);
    std::fprintf(stdout, "SUBT0_COMBOS %d\n", combos);
    std::fprintf(stdout, "SUBT0_MAX_ERR_OVER_RADIUS %.6f\n", worst);
  }

  // ---- L4: the two derivations must agree on the overlap band ------------
  // The series path (D1) and the log Gamma path (D8) share no coefficient, no
  // truncation argument and no remainder bound. On [t0, 2 t0] both are
  // defined, so their enclosures must intersect. This is the only layer that
  // checks a certified radius against something other than a corpus or a
  // transcription of its own derivation.
  {
    double worst = 0.0;
    int checked = 0;
    for (double h : {200.0, 210.0, 250.0, 300.0, 350.0, 400.0}) {
      for (mpfr_prec_t pr : {(mpfr_prec_t)128, (mpfr_prec_t)256}) {
        const Ball a = theta_certified(h, pr);
        const Ball b = theta_certified_loggamma(h, pr);
        mpfr_t d;
        mpfr_init2(d, 512);
        mpfr_sub(d, a.centre(), b.centre(), MPFR_RNDN);
        mpfr_abs(d, d, MPFR_RNDN);
        const double combined = a.radius() + b.radius();
        const double ratio = mpfr_get_d(d, MPFR_RNDU) / combined;
        mpfr_clear(d);
        if (ratio > worst) worst = ratio;
        ZF_CHECK(ratio <= 1.0);
        ++checked;
      }
    }
    ZF_CHECK(checked == 12);
    std::fprintf(stdout, "OVERLAP_CHECKED %d\n", checked);
    std::fprintf(stdout, "OVERLAP_MAX_DIFF_OVER_COMBINED %.6f\n", worst);
  }

  // ---- L6: the Stirling sector invariant (ATTACKS.md row 21) -------------
  // Re(z + m) >= Im(z + m), i.e. |arg w| <= pi/4. Asserted directly against
  // the shift the implementation would use, rather than trusting that the rule
  // establishing it is still in theta_shift. This is a tightness and
  // validated-range invariant, not a soundness one: the Stieltjes bound holds
  // throughout Re w > 0 (MATHS.md D8.4). Without this layer, deleting the
  // sector term costs 187x of certified radius at t = 199.999 and nothing in
  // the suite notices.
  {
    int checked = 0, violations = 0;
    double worst_arg_over_pi = 0.0;
    for (double t : {1e-6, 0.5, 1.0, 5.0, 14.1347251417, 50.0, 100.0, 150.0,
                     199.999, 250.0, 400.0}) {
      for (mpfr_prec_t pr : {(mpfr_prec_t)128, (mpfr_prec_t)192,
                             (mpfr_prec_t)256, (mpfr_prec_t)512}) {
        const unsigned long m = zetaforge::theta_loggamma_shift(t, pr);
        const double w_re = 0.25 + static_cast<double>(m);
        const double w_im = t / 2.0;
        const double arg_over_pi = std::atan2(w_im, w_re) / M_PI;
        if (arg_over_pi > worst_arg_over_pi) worst_arg_over_pi = arg_over_pi;
        if (!(w_im <= w_re)) {
          ++violations;
          std::printf("SECTOR_VIOLATION t=%.17g prec=%d m=%lu arg/pi=%.6f\n",
                      t, (int)pr, m, arg_over_pi);
        }
        ++checked;
      }
    }
    ZF_CHECK(violations == 0);
    ZF_CHECK(checked == 44);
    std::fprintf(stdout, "SECTOR_CHECKED %d violations %d max_arg_over_pi %.6f\n",
                 checked, violations, worst_arg_over_pi);
  }

  // ---- L5: Bernoulli oracle against FLINT (ATTACKS.md row 3) -------------
  // The recurrence in core/src/bernoulli.cpp feeds both certified series. It is
  // checked against the ten values transcribed in em_eval.cpp internally, and
  // here against FLINT's own table, which shares no code with either.
  {
    mpq_t ours;
    mpq_init(ours);
    fmpq_t theirs;
    fmpq_init(theirs);
    mpq_t theirs_q;
    mpq_init(theirs_q);
    int mismatches = 0, checked = 0;
    for (unsigned n : {1u, 2u, 3u, 5u, 8u, 10u, 17u, 31u, 64u, 100u, 128u}) {
      zetaforge::bernoulli_2n(n, ours);
      bernoulli_fmpq_ui(theirs, 2 * n);
      fmpq_get_mpq(theirs_q, theirs);
      if (mpq_cmp(ours, theirs_q) != 0) {
        ++mismatches;
        std::printf("BERNOULLI_MISMATCH n=%u\n", n);
      }
      ++checked;
    }
    mpq_clear(ours);
    mpq_clear(theirs_q);
    fmpq_clear(theirs);
    ZF_CHECK(mismatches == 0);
    ZF_CHECK(checked == 11);
    std::fprintf(stdout, "BERNOULLI_ORACLE checked=%d mismatches=%d\n",
                 checked, mismatches);
  }

  // ---- L3: policy equality ---------------------------------------------
  for (double h : {200.0, 1000.0, 3.0e12}) {
    const Ball th = theta_certified(h, kPrec);
    const double expect = radius_transcribed(h, kPrec);
    ZF_CHECK(std::fabs(th.radius() - expect) <= 1e-3 * expect);
  }

  std::fprintf(stdout, "THETA_SUITE failures %d\n", ::zftest::failure_count());
  return ::zftest::failure_count() == 0 ? 0 : 1;
}

// An exception escaping the suite is a failure, not a crash. Without this the
// Bernoulli tripwire (ATTACKS.md row 3) took the process down with SIGABRT,
// which is a worse diagnostic than a named failure and does not even produce a
// stable exit code for the ledger to assert against.
int main() {
  try {
    return run_suite();
  } catch (const std::exception& e) {
    std::fprintf(stderr, "ZF_CHECK failed: theta suite threw (%s) [seed %llx]\n",
                 e.what(),
                 static_cast<unsigned long long>(::zftest::current_seed()));
    std::fprintf(stdout, "THETA_SUITE aborted by exception; failures %d\n",
                 ::zftest::failure_count() + 1);
    return 1;
  }
}
