#pragma once

// Certified Riemann-Siegel theta via asymptotic series. Closes MATHS.md
// obligations D1 (truncation bound) and D2 (RS validity threshold and working
// precision derivation).
//
// Series (coefficients transcribed from oracle discovery against mpmath
// loggamma at 140+ digits; see tools/discover_theta_coefficients.py):
//   theta(t) = (t/2)ln(t/2pi) - t/2 - pi/8 + sum_{k>=1} c_k / t^(2k-1)
//
// Validity: t >= kThetaTMin = 200, pinned from Gabcke's published remainder
// conditions (Gabcke 1979, Thm 1 p.139, as quoted in Arias de Reyna,
// Math. Comp. 80 (2011) 995-1009). Below kThetaTMin the function throws:
// the Euler-Maclaurin path owns that range (stage 4, gamma_1 reproduction).
//
// Radius claim (D1): |true - centre| <= radius where radius carries the
// certified remainder bound |c7|/t^13 + |c8|/t^15 times kThetaSafety (4)
// plus mpfr rounding slack at the working precision. The factor 4 headroom
// stands in for Gabcke's exact per-term constants; transcribing those to
// shrink it is MATHS.md open item O1.

#include <mpfr.h>

#include "ball.hpp"

namespace zetaforge {

constexpr double kThetaTMin = 200.0;
constexpr int kThetaTerms = 6;
constexpr double kThetaSafety = 4.0;

// Throws std::domain_error for t < kThetaTMin or non-finite t.
Ball theta_certified(double t, mpfr_prec_t prec);

}  // namespace zetaforge
