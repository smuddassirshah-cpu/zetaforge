#pragma once

// Euler-Maclaurin evaluation of Z(t) = e^{i theta(t)} zeta(1/2 + i t)
// with certified radii. Unified path for t in (0, kEmTMax]:
//   - gamma_1 reproduction (t ~ 14.13)
//   - the full EM/RS overlap band
//   - everything below the RS-validity threshold t0 = 200
//
// Method: partial sum S_N = sum_{n<N} n^{-s}, EM tail with M Bernoulli
// correction terms (exact rationals from bernoulli.hpp, parsed at working
// precision), remainder bounded by BACKLUND's explicit bound (Edwards,
// Riemann's Zeta Function, section 6.4): first omitted term times
// |s + 2M + 1| / (sigma + 2M + 1), both factors as balls. N and M are pinned
// from the target radius. Then Z = u cos(theta) - v sin(theta) with
// theta_certified, and an explicit straddle classification:
//
//   Certified : the value ball does not contain zero
//   Contested : |Re Z| <= Re-radius -> the sign of Z cannot be decided from
//               this evaluation alone; downstream sign logic MUST escalate.
//               A zero-straddling ball can never be silently signed.
//
// Radius components carried explicitly (see MATHS.md D8.8 and D8.9):
//   (i)    EM tail remainder, Backlund's bound as a ball
//   (ii)   mpfr rounding across the whole evaluation
//   (iii)  theta radius propagated through cos(theta) and sin(theta)
//   (iv)   coefficient representation error (Bernoulli rationals parsed at p)
//
// There is no safety factor. The rev-5 policy multiplied the first omitted
// term by kEmSafety = 4 and checked at runtime that the term sequence had not
// yet turned over. That is not a theorem for s = 1/2 + it: the enveloping
// property justifying first-omitted-term bounds holds for real s > 0, and on
// the critical line the true remainder over first omitted term was measured at
// 4.72 (t = 200), 13.98 (t = 1000) and 81.3 (t = 20000) at cost-minimal N, so
// the policy under-reported by up to 20x on the band this path owns. The
// monotone check survives only as a sanity assertion that can neither widen
// nor narrow a radius: it detects that the series has not passed its
// divergence turnover, which bounds nothing.

#include <mpfr.h>

#include "ball.hpp"
#include "theta.hpp"

namespace zetaforge {

// Derived in MATHS.md D8.10, replacing an asserted 20000. The EM path owes
// only 0 < t <= t0 = 200, where the RS path cannot serve; above t0 it is a
// second opinion, at a cost linear in t against the RS path's sqrt(t/2pi).
// 2 t0 buys a full octave of overlap for the agreement layer and stops, since
// overlap evidence does not improve with height.
constexpr double kEmTMax = 400.0;

enum class ZStatus { Certified, Contested };

struct ZResult {
  Ball re;       // Z(t) itself: a REAL certified ball, which is the whole point
  // Diagnostic, not part of the certified value. Z(t) is real for real t, so
  // Im[e^{i theta} zeta] must be a ball CONTAINING ZERO (MATHS.md D8.9). It is
  // a free simultaneous check on theta's branch, on the sign convention of the
  // assembly and on the zeta ball, and test_zeta layer L-C asserts it.
  Ball im;
  ZStatus status;
  int em_n = 0;            // pinned Dirichlet length
  int em_m = 0;            // pinned number of Bernoulli correction terms
  double em_tail_bound = 0.0;  // Backlund bound at (em_n, em_m); test_zeta
                               // layer L-D transcribes it independently
};

// Evaluates Z(t) for t > 0 (critical line). Throws std::invalid_argument
// for non-finite/non-positive t and std::domain_error for t > kEmTMax
// (the RS path owns that range once its certified correction lands).
ZResult zeta_em(double t, mpfr_prec_t prec);

}  // namespace zetaforge
