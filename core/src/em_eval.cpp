#include "zetaforge/em_eval.hpp"

#include <cmath>
#include <cstdlib>
#include <stdexcept>

namespace zetaforge {

namespace {

constexpr int kBernMaxK = 10;  // B_2 .. B_20 exact rationals
constexpr long long kBNum[kBernMaxK] = {
    1, -1, 1, -1, 5, -691, 7, -3617, 43867, -174611};
constexpr long long kBDen[kBernMaxK] = {
    6, 30, 42, 30, 66, 2730, 6, 510, 798, 330};

int m_delta_env() {
  const char* e = std::getenv("ZF_EM_M_DELTA");
  return e ? static_cast<int>(std::strtol(e, nullptr, 10)) : 0;
}

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
  // D8 derivation required before implementation lands. See MATHS.md.
  throw std::runtime_error(
      "zeta_em: EM evaluation not yet implemented; requires D8 derivation "
      "(certified theta below t0) and EM tail remainder bound");
}

}  // namespace zetaforge
