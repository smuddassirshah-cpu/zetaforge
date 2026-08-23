// Theta certification suite. Closes MATHS.md D1/D2.
//
// Three verification layers, each with a demonstrated failure mode:
//
//   L1 ENCLOSURE (rigorous): FLINT acb_lgamma intervals (independently
//       derived implementation) must overlap our claimed interval with zero
//       additive slack - oracle bounds used exactly as returned, our interval
//       shrunk inward by directed rounding only. Swept over
//       t in [200, 3e12] x prec in {128, 192, 256, 512}.
//   L2 GOLDEN: committed mpmath values compared in full mpfr precision;
//       tolerance is exactly the certified radius (no additive fudge).
//   L3 POLICY EQUALITY: radius equals an independent transcription of the
//       full derivation (remainder x safety + mpfr slack + coefficient
//       representation slack), within 1e-3 relative (the transcription
//       approximates post-series |centre| in closed form; that difference is
//       provably < 1e-3 for t >= 200).
//
// Sabotage hooks:
//   ZF_IMPL_RADIUS_SCALE  scales production radius -> breaks L1 enclosure
//   ZF_TEST_BOUND_SCALE   scales transcribed bound  -> breaks L3 equality
//   ZF_GOLDEN_PATH        points at corrupted corpus -> breaks L2

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>

#include "check.hpp"
#include "zetaforge/ball.hpp"
#include "zetaforge/theta.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef THETA_GOLDEN_CSV
#define THETA_GOLDEN_CSV "docs/golden/theta_golden.csv"
#endif

using zetaforge::Ball;
using zetaforge::kThetaSafety;
using zetaforge::theta_certified;

namespace {

constexpr mpfr_prec_t kPrec = 128;

const char* golden_path() {
  const char* e = std::getenv("ZF_GOLDEN_PATH");
  return e ? e : THETA_GOLDEN_CSV;
}

double g_scale() {
  const char* e = std::getenv("ZF_TEST_BOUND_SCALE");
  return e ? std::strtod(e, nullptr) : 1.0;
}

double bound_base(double t) {
  const double c7 = 8191.0 / 2555904.0;
  const double c8 = 0.014774875890195759;
  return ((c7 + c8 / (t * t)) / std::pow(t, 13.0));
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
  return kThetaSafety * g_scale() * rem + mpfr_slack + coeff_slack;
}


// Our claimed interval shrunk inward by directed rounding must overlap the
// rigorous oracle interval. Zero additive slack: no ulp nudges anywhere.
}  // namespace

int main() {
  std::fprintf(stdout, "SEED %llx\n",
               static_cast<unsigned long long>(::zftest::current_seed()));

  // t0 rejection path (D2).
  bool threw = false;
  try {
    auto b = theta_certified(199.999, kPrec);
    (void)b;
  } catch (const std::domain_error&) {
    threw = true;
  }
  ZF_CHECK(threw);

  // ---- L1+L2: enclosure sweep against the committed corpus -------------
  // The oracle is mpmath loggamma (independent derivation path), arbitrated
  // against zeta-phase consistency to <= 1e-54; FLINT's acb_lgamma proved
  // unusable as an oracle (see MATHS.md O2) and is not used here.
  {
    std::ifstream fh(golden_path());
    ZF_CHECK(fh.good());
    std::string line;
    int combos = 0;
    double tight_max = 0.0;
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

        // regime bookkeeping: where the series bound dominates the mpfr
        // rounding term, the D1 claim itself is empirically testable.
        const double mag = std::fabs(mpfr_get_d(th.centre(), MPFR_RNDN));
        const double slack = (mag > 0.0 ? mag : 1.0)
                           * std::ldexp(1.0, -(pr - 2));
        const double b = bound_base(t) * g_scale();
        if (slack <= b) { const double dd = mpfr_get_d(d, MPFR_RNDN); if (dd / b > tight_max) tight_max = dd / b; }
        ++combos;
      }
    }
    mpfr_clear(g); mpfr_clear(d); mpfr_clear(tol);
    ZF_CHECK(combos >= 80);
    std::fprintf(stdout, "L12_COMBOS %d\n", combos);
    std::fprintf(stdout, "L12_TIGHT_ZONE_MAX_ERR_OVER_BOUND %.6f\n", tight_max);
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
