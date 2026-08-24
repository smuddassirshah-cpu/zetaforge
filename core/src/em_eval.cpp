#include "zetaforge/em_eval.hpp"

#include <cmath>
#include <stdexcept>

#include <gmp.h>

namespace zetaforge {

namespace {

constexpr int kBernMaxK = 10;
constexpr long long kBNum[kBernMaxK] = {
    1, -1, 1, -1, 5, -691, 7, -3617, 43867, -174611};
constexpr long long kBDen[kBernMaxK] = {
    6, 30, 42, 30, 66, 2730, 6, 510, 798, 330};

// EM correction order, fixed at compile time. An environment-tunable order
// (the removed ZF_EM_M_DELTA) put the numerical configuration outside the
// config hash that PLAN.md section 8 makes the unit of the determinism
// contract: the same defect class that had ZF_EM_STATUS_FORCE removed at
// rev 1. Readiness review finding C4.
constexpr int kEmCorrectionOrder = 8;
static_assert(kEmCorrectionOrder >= 2 && kEmCorrectionOrder <= kBernMaxK,
              "EM correction order must index the Bernoulli table");

}  // namespace

ZResult zeta_em(double t, mpfr_prec_t prec) {
  if (!std::isfinite(t) || t <= 0.0) {
    throw std::invalid_argument("zeta_em requires finite positive t");
  }
  if (t > kEmTMax) {
    throw std::domain_error(
        "zeta_em above kEmTMax: RS path with certified correction owns "
        "this range");
  }
  // Implementation requires the D8 rewrite (certified sub-t0 theta) and a
  // sound EM tail remainder bound. Both are Phase 2 work per
  // docs/REVIEW-2026-08-24.md findings A1 and A2: the |zeta| plan in D8
  // cannot certify a sign change, and the first-omitted-term policy is not a
  // theorem on the critical line.
  throw std::runtime_error(
      "zeta_em: not yet implemented; blocked on D8 sub-t0 derivation "
      "and EM tail remainder monotone-check derivation");
}

}  // namespace zetaforge
