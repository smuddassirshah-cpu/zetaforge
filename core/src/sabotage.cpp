// Compiled into zetaforge_core ONLY when ZF_SABOTAGE_HOOKS is on. This is the
// single file in the certified tree allowed to read the environment; see
// core/include/zetaforge/sabotage.hpp for the contract and CI enforcement.

#include "zetaforge/sabotage.hpp"

#ifdef ZF_SABOTAGE_HOOKS

#include <cstdlib>

namespace zetaforge {

double zf_radius_sabotage_scale() noexcept {
  const char* e = std::getenv("ZF_IMPL_RADIUS_SCALE");
  if (e == nullptr) {
    return 1.0;
  }
  const double v = std::strtod(e, nullptr);
  // A non-positive or unparsable value would widen or void the radius, which
  // could mask a defect rather than expose one. Refuse it.
  return v > 0.0 ? v : 1.0;
}

}  // namespace zetaforge

#endif  // ZF_SABOTAGE_HOOKS
