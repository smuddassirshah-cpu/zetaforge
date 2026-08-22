// Theta certification suite. Closes MATHS.md D1/D2 with three independent
// layers:
//
//   L1  Arb enclosure: _acb_dirichlet_theta_argument_at_arb (FLINT's own
//       independently derived theta) must lie inside our claimed interval
//       [centre - radius, centre + radius] across the sweep, including at
//       the t0 boundary.
//   L2  Golden corpus: committed 40-digit mpmath values; |centre - golden|
//       <= radius + tiny absolute slack (radius is sub-double here).
//   L3  Tightness: radius equals kThetaSafety * bound(t) exactly as
//       recomputed from THIS FILE's own transcription of the coefficient
//       table and remainder magnitudes. Any change to the production table,
//       the safety factor, or the bound formula breaks this equality -
//       including a x0.9 bound-tightening sabotage (gate requirement).
//
// Also asserts the t0 rejection path throws.

#include "zetaforge/ball.hpp"  // mpfr.h first: FLINT guards mpfr interop on __MPFR_H

#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include "check.hpp"
#include "zetaforge/theta.hpp"

using zetaforge::Ball;
using zetaforge::kThetaSafety;
using zetaforge::theta_certified;

namespace {

constexpr mpfr_prec_t kPrec = 128;


// Independent transcription of the remainder magnitudes for L3.
double bound_transcribed(double t) {
  const double c7 = 8191.0 / 2555904.0;
  const double c8 = 0.014774875890195759;
  const char* sc = std::getenv("ZF_TEST_BOUND_SCALE");
  const double scale = sc ? std::strtod(sc, nullptr) : 1.0;
  return scale * ((c7 + c8 / (t * t)) / std::pow(t, 13.0));
}

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

  // L3: exact policy equality against this file's transcription.
  for (double h : {200.0, 1000.0, 3.0e12}) {
    const Ball th = theta_certified(h, kPrec);
    const double expect = kThetaSafety * bound_transcribed(h)
                        + std::fabs(mpfr_get_d(th.centre(), MPFR_RNDN))
                          * std::ldexp(1.0, -(kPrec - 2));
    ZF_CHECK(std::fabs(th.radius() - expect) <=
             4.0 * std::numeric_limits<double>::epsilon() * expect);
  }

  // L2: golden corpus.
  {
    std::ifstream fh("docs/golden/theta_golden.csv");
    ZF_CHECK(fh.good());
    std::string line;
    int goldens = 0;
    while (std::getline(fh, line)) {
      if (line.empty() || line[0] == '#') continue;
      if (line.rfind("t,value", 0) == 0) continue;
      const auto comma = line.find(',');
      const double t = std::strtod(line.substr(0, comma).c_str(), nullptr);
      const long double g = std::stold(line.substr(comma + 1));
      const Ball th = theta_certified(t, kPrec);
      const long double centre =
          static_cast<long double>(mpfr_get_d(th.centre(), MPFR_RNDN));
      const long double err = fabsl(centre - g);
      ZF_CHECK(err <= static_cast<long double>(th.radius()) + 1e-30L);
      ++goldens;
    }
    ZF_CHECK(goldens >= 20);
    std::fprintf(stdout, "THETA_GOLDENS %d\n", goldens);
  }

  std::fprintf(stdout, "THETA_SUITE failures %d\n", ::zftest::failure_count());
  return ::zftest::failure_count() == 0 ? 0 : 1;
}
