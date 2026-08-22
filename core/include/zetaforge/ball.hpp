#pragma once

// Decision note: production ball arithmetic per DECISIONS.md (stage 1 gate,
// S4): our own implementation, Arb (as merged into FLINT 3) is oracle and
// escalation engine only, never inside the certified hot path.
//
// Representation: centre as mpfr at fixed working precision, radius as a
// non-negative double. Enclosure invariant: the true value lies in
// [centre - radius, centre + radius] whenever radius is finite.
//
// Radius discipline:
//   - every radius computation rounds outward via zetaforge/radius.hpp
//   - centre rounding error enters through half_ulp_bound of the rounded result
//   - no binary operation claims exactness: results carry at least one ulp
//     even when inputs are exact scalars
//   - a radius of INFINITY means "unknown at this precision": needs_escalation()
//
// Directed conversions from mpfr magnitudes to double ceilings keep the
// enclosure sound across the precision boundary; zero ceilings are clamped up
// to denorm_min so a huge radius times a tiny magnitude cannot be dropped.

#include <cmath>
#include <cstddef>
#include <string>

#include <mpfr.h>

namespace zetaforge {

class Ball {
 public:
  explicit Ball(mpfr_prec_t prec_bits = 128);
  Ball(const Ball& other);
  Ball(Ball&& other) noexcept;
  Ball& operator=(const Ball& other);
  Ball& operator=(Ball&& other) noexcept;
  ~Ball();

  // Exact scalar: doubles are representable exactly in mpfr, radius stays 0.
  static Ball from_double(double x, mpfr_prec_t prec_bits);

  // Sub-double-granularity certified values (theta series): the caller owns
  // the complete error budget; finish_radius's double-ulp policy would
  // swamp radii of order 1e-25 at campaign heights.
  static Ball from_centre_and_radius(mpfr_srcptr centre, double radius);

  // Parses a decimal string. The stored centre is the correctly rounded
  // representation at prec_bits; the parse itself is inexact unless the value
  // is representable, so radius starts at half an ulp of the stored centre.
  static Ball parse(const std::string& decimal, mpfr_prec_t prec_bits);

  static Ball add(const Ball& a, const Ball& b);
  static Ball sub(const Ball& a, const Ball& b);
  static Ball mul(const Ball& a, const Ball& b);
  // Exact integer scaling: n must fit exactly in a double.
  static Ball scale(const Ball& a, long n);

  mpfr_prec_t precision() const { return mpfr_get_prec(centre_); }
  double radius() const { return radius_; }
  mpfr_srcptr centre() const { return centre_; }

  bool contains_zero() const;
  // INFINITY means the enclosure is trivially wide: unknown at this precision.
  bool unknown_at_precision() const { return !(radius_ < INFINITY); }
  // Relative width policy hook for the escalation ladder (stage 4+ consumers).
  bool needs_escalation(double max_rel_width) const;

  void widen_radius(double extra);

  std::string centre_decimal(size_t digits) const;

 private:
  mpfr_t centre_;
  double radius_;

  void init(mpfr_prec_t prec_bits);
  // Radius assembly shared by add/sub/mul: outward-rounded term sum plus the
  // centre rounding bound, inflated so no binary op claims exactness.
  static double finish_radius(double terms, mpfr_srcptr rounded_centre);
  static double abs_centre_ceiling(mpfr_srcptr c);
};

}  // namespace zetaforge
