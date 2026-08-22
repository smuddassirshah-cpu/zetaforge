#include <cmath>
#include <limits>
#include <stdexcept>

#include "check.hpp"
#include "zetaforge/ball.hpp"

int main() {
  using zetaforge::Ball;

  const Ball b{0.5, 0.25};
  ZF_CHECK(b.centre() == 0.5);
  ZF_CHECK(b.radius() == 0.25);
  ZF_CHECK(!b.contains_zero());
  ZF_CHECK(b.width() == 0.5);

  const Ball z{-0.1, 0.2};
  ZF_CHECK(z.contains_zero());
  ZF_CHECK(std::abs(z.width() - 0.4) < 1e-15);

  bool threw = false;
  try {
    const Ball bad{1.0, -0.5};
    (void)bad;
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  ZF_CHECK(threw);

  bool threw_nan_centre = false;
  try {
    const Ball nan_centre{std::nan(""), 0.5};
    (void)nan_centre;
  } catch (const std::invalid_argument&) {
    threw_nan_centre = true;
  }
  ZF_CHECK(threw_nan_centre);

  bool threw_inf_centre = false;
  try {
    const Ball inf_centre{std::numeric_limits<double>::infinity(), 0.5};
    (void)inf_centre;
  } catch (const std::invalid_argument&) {
    threw_inf_centre = true;
  }
  ZF_CHECK(threw_inf_centre);

  bool threw_inf_radius = false;
  try {
    const Ball inf_radius{0.5, std::numeric_limits<double>::infinity()};
    (void)inf_radius;
  } catch (const std::invalid_argument&) {
    threw_inf_radius = true;
  }
  ZF_CHECK(threw_inf_radius);

  return ::zftest::failure_count() == 0 ? 0 : 1;
}
