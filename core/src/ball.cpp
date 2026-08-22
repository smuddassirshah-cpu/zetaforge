#include "zetaforge/ball.hpp"

#include <cmath>
#include <stdexcept>

namespace zetaforge {

Ball::Ball(double centre, double radius) : centre_(centre), radius_(radius) {
  if (radius < 0.0 || !std::isfinite(radius)) {
    throw std::invalid_argument("ball radius must be finite and non-negative");
  }
}

double Ball::centre() const noexcept { return centre_; }

double Ball::radius() const noexcept { return radius_; }

bool Ball::contains_zero() const noexcept {
  return std::abs(centre_) <= radius_;
}

double Ball::width() const noexcept { return 2.0 * radius_; }

}  // namespace zetaforge
