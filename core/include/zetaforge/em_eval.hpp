#pragma once

// Euler-Maclaurin evaluation of Z(t) = e^{i theta(t)} zeta(1/2 + i t)
// with certified radii. Unified path for t in (0, kEmTMax]:
//   - gamma_1 reproduction (t ~ 14.13)
//   - the full EM/RS overlap band
//   - everything below the RS-validity threshold t0 = 200
//
// Method: partial sum S_N = sum_{n<N} n^{-s}, EM tail with M Bernoulli
// correction terms (exact rational coefficients parsed at working
// precision), remainder bounded by first-omitted magnitude times kEmSafety,
// functional-equation assembly with theta_certified, and an explicit
// straddle classification:
//
//   Certified : the value ball does not contain zero
//   Contested : |Re Z| <= Re-radius -> the sign of Z cannot be decided from
//               this evaluation alone; downstream sign logic MUST escalate.
//               A zero-straddling ball can never be silently signed.
//
// Radius components carried explicitly (see MATHS.md D-EM):
//   (i)    EM tail remainder (first omitted x SAFETY, monotone-checked)
//   (ii)   mpfr rounding across the whole evaluation
//   (iii)  theta radius propagated through e^{i theta}
//   (iv)   coefficient representation error (Bernoulli rationals parsed at p)

#include <mpfr.h>

#include "ball.hpp"
#include "theta.hpp"

namespace zetaforge {

constexpr double kEmTMax = 20000.0;
constexpr double kEmSafety = 4.0;

enum class ZStatus { Certified, Contested };

struct ZResult {
  Ball re;       // real part of Z as a certified ball
  ZStatus status;
};

// Evaluates Z(t) for t > 0 (critical line). Throws std::invalid_argument
// for non-finite/non-positive t and std::domain_error for t > kEmTMax
// (the RS path owns that range once its certified correction lands).
ZResult zeta_em(double t, mpfr_prec_t prec);

}  // namespace zetaforge
