#pragma once

// Decision note: sabotage hooks are the falsifiability channel the gate
// battery runs against. Two properties matter more than convenience, and both
// are structural rather than a matter of discipline:
//
//   1. A production binary cannot reach a hook. With ZF_SABOTAGE_HOOKS off,
//      the scale below is a compile-time inline 1.0 constant, the definition
//      in core/src/sabotage.cpp is not compiled at all, and no path in the
//      library reads the environment.
//   2. A hooked binary cannot lose its hooks silently. The CI leg
//      "sabotage-battery" configures with hooks on and REQUIRES the battery to
//      detect a scaled radius; the same battery requires the default build not
//      to respond. Review finding C2 recorded the previous wiring, in which
//      the define was applied to a test target while the hook lived inside the
//      library, so the knob could never reach it and the recorded stage 3
//      battery was unreproducible.
//
// core/src/sabotage.cpp is the ONLY file under core/src or core/include
// permitted to read the environment at all. The CI job "no-env-knobs" greps
// the production tree for environment reads and fails on any hit outside that
// single gated unit, so this rule cannot decay into a convention.

namespace zetaforge {

#ifdef ZF_SABOTAGE_HOOKS
// Multiplier applied to a certified radius before it is published. Returns
// 1.0 unless ZF_IMPL_RADIUS_SCALE is set to a positive value.
double zf_radius_sabotage_scale() noexcept;
#else
inline double zf_radius_sabotage_scale() noexcept { return 1.0; }
#endif

}  // namespace zetaforge
