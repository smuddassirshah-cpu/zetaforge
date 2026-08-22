#include <cassert>
#include <cmath>
#include <stdexcept>

#include "zetaforge/ball.hpp"

int main() {
  using zetaforge::Ball;

  const Ball b{0.5, 0.25};
  assert(b.centre() == 0.5);
  assert(b.radius() == 0.25);
  assert(!b.contains_zero());
  assert(b.width() == 0.5);

  const Ball z{-0.1, 0.2};
  assert(z.contains_zero());
  assert(std::abs(z.width() - 0.4) < 1e-15);

  bool threw = false;
  try {
    const Ball bad{1.0, -0.5};
    (void)bad;
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  assert(threw);

  return 0;
}
