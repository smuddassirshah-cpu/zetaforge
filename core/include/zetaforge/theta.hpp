#pragma once

// Certified Riemann-Siegel theta via asymptotic series. Closes MATHS.md
// obligations D1 (truncation bound) and D2 (RS validity threshold and working
// precision derivation).
//
// Series (coefficients transcribed from oracle discovery against mpmath
// loggamma at 140+ digits; see tools/discover_theta_coefficients.py):
//   theta(t) = (t/2)ln(t/2pi) - t/2 - pi/8 + sum_{k>=1} c_k / t^(2k-1)
//
// Validity: the SERIES is used for t >= kThetaTMin = 200, pinned from Gabcke's
// published remainder conditions (Gabcke 1979, Thm 1 p.139, as quoted in Arias
// de Reyna, Math. Comp. 80 (2011) 995-1009). t0 is a dispatch switch, not a
// domain floor: below it theta_certified evaluates the log Gamma path of
// MATHS.md D8 (Gamma recurrence into the validated Stirling sector, Stieltjes
// remainder as a certified radius) and returns a certified ball. The function
// is defined for every finite t > 0 and throws only for t <= 0 or non-finite
// input.
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

// Throws std::domain_error for t <= 0 and std::invalid_argument for
// non-finite t.
Ball theta_certified(double t, mpfr_prec_t prec);

// The log Gamma derivation on its own, for every finite t > 0 including the
// range where theta_certified dispatches to the series. Exposed so the two
// independent derivations can be compared against each other on the overlap
// band [t0, 2 t0]: agreement within combined radii is evidence neither the
// corpus nor a self-transcription can supply, because the two paths share no
// coefficient, no truncation argument and no remainder bound. Production
// callers want theta_certified.
Ball theta_certified_loggamma(double t, mpfr_prec_t prec);

}  // namespace zetaforge
