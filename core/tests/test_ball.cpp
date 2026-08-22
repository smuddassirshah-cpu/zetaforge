#include <cmath>
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

  return ::zftest::failure_count() == 0 ? 0 : 1;
}
