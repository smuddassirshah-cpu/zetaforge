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
// Radius claim (D1, proven by D1b): |true - centre| <= radius where radius
// carries the PROVEN series remainder bound
//     c7/t^13 + (|B16|/15)/t^15 + (1/2) e^{-pi t}
// (Nemes, Appl. Anal. Discrete Math. 7 (2013), Thm 4, after the exact
// reduction of the series remainder to Hermite's expansion at arg z = pi/2;
// MATHS.md D1b) plus mpfr rounding slack at the working precision and the
// coefficient representation term. There is no safety factor anywhere in
// theta: the empirical kThetaSafety = 4 of stage 3 was retired at rev 7 when
// O1 closed, superseded by a bound both proven and tighter.

#include <mpfr.h>

#include "ball.hpp"

namespace zetaforge {

constexpr double kThetaTMin = 200.0;
constexpr int kThetaTerms = 6;

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

// The Gamma-recurrence shift m the log Gamma path would use at (t, prec).
// Exposed so the suite can assert the sector invariant Re(z + m) >= Im(z + m)
// directly, rather than trusting that the rule which establishes it is still
// there. The Stieltjes remainder bound is sound throughout Re w > 0, so this
// is a TIGHTNESS and validated-range invariant, not a soundness one; see
// MATHS.md D8.4 and D8.5.
unsigned long theta_loggamma_shift(double t, mpfr_prec_t prec);

}  // namespace zetaforge
