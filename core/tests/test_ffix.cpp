// Property suite for Q64.64 fixed-point multiplication with tracked error.
// The under-reporting hunt (stage 2 attack item 6): the tracked bound must be
// >= the true error measured against an exact 256-bit reference, including on
// adversarial magnitudes near the range limit.

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

#include "check.hpp"
#include "zetaforge/ffix.hpp"

using zetaforge::Ffix;

namespace {

struct U256 {
  unsigned __int128 hi;
  unsigned __int128 lo;
};

U256 mul_u128(unsigned __int128 a, unsigned __int128 b) {
  const uint64_t ah = static_cast<uint64_t>(a >> 64);
  const uint64_t al = static_cast<uint64_t>(a);
  const uint64_t bh = static_cast<uint64_t>(b >> 64);
  const uint64_t bl = static_cast<uint64_t>(b);
  const unsigned __int128 ll = static_cast<unsigned __int128>(al) * bl;
  unsigned __int128 mid = static_cast<unsigned __int128>(al) * bh;
  mid += static_cast<unsigned __int128>(ah) * bl;
  const unsigned __int128 hh = static_cast<unsigned __int128>(ah) * bh;
  U256 out;
  out.lo = ll + (mid << 64);
  out.hi = hh + (mid >> 64) + (out.lo < ll ? 1 : 0);
  return out;
}

unsigned __int128 abs128(Ffix::raw_t v) {
  return v < 0 ? static_cast<unsigned __int128>(-v) : static_cast<unsigned __int128>(v);
}

}  // namespace

int main() {
  std::fprintf(stdout, "SEED %llx\n",
               static_cast<unsigned long long>(::zftest::current_seed()));
  auto& rng = ::zftest::rng();

  // Exact small-value checks against int arithmetic.
  {
    const Ffix two = Ffix::from_int(2);
    const Ffix three = Ffix::from_int(3);
    ZF_CHECK(two.add(three).raw() == (static_cast<Ffix::raw_t>(5) << 64));
    ZF_CHECK(two.mul(three).raw() == (static_cast<Ffix::raw_t>(6) << 64));
    ZF_CHECK(two.mul(three).err() >= 1);
    ZF_CHECK(two.sub(three).negate().raw() == three.sub(two).raw());
  }

  // Randomised truncation audit: the value must sit within one unit of the
  // exact quotient, and the tracked bound must cover that plus policy slack.
  constexpr int kTrials = 5000;
  for (int t = 0; t < kTrials; ++t) {
    const auto mag_a = static_cast<unsigned __int128>(rng.next()) >> 28;  // < 2^100
    const auto mag_b = static_cast<unsigned __int128>(rng.next()) >> 28;
    const bool neg_a = rng.next() & 1;
    const bool neg_b = rng.next() & 1;

    const Ffix::raw_t ra = neg_a ? -static_cast<Ffix::raw_t>(mag_a)
                                 : static_cast<Ffix::raw_t>(mag_a);
    const Ffix::raw_t rb = neg_b ? -static_cast<Ffix::raw_t>(mag_b)
                                 : static_cast<Ffix::raw_t>(mag_b);

    const Ffix fa = Ffix::from_raw(ra);
    const Ffix fb = Ffix::from_raw(rb);
    const Ffix prod = fa.mul(fb);

    const U256 m = mul_u128(mag_a, mag_b);
    const unsigned __int128 q_floor = (m.hi << 64) | (m.lo >> 64);
    const uint64_t frac = static_cast<uint64_t>(m.lo);

    // Magnitude result must be exactly the truncated quotient: the dropped
    // fraction is tracked in the error bound, never folded into the value.
    const unsigned __int128 out_mag = prod.raw() < 0
                                          ? static_cast<unsigned __int128>(-prod.raw())
                                          : static_cast<unsigned __int128>(prod.raw());
    ZF_CHECK(out_mag == q_floor);

    // Under-report hunt: the tracked bound must cover at least the truncation
    // unit when a fraction was dropped, plus the never-exact policy unit.
    const Ffix::raw_t min_bound = 1 + (frac != 0 ? 1 : 0);
    ZF_CHECK(prod.err() >= min_bound);

    // Monotone propagation: chaining through an error-carrying operand must
    // never shrink the bound.
    if (fa.err() > 0) {
      const Ffix again = prod.mul(fa);
      ZF_CHECK(again.err() >= prod.err());
    }
  }

  // Range enforcement.
  bool threw = false;
  try {
    const Ffix::raw_t big = static_cast<Ffix::raw_t>(1) << 121;
    auto f = Ffix::from_raw(big);
    (void)f;
  } catch (const std::overflow_error&) {
    threw = true;
  }
  ZF_CHECK(threw);

  std::fprintf(stdout, "FFIX_TRIALS %d failures %d\n", kTrials,
               ::zftest::failure_count());
  return ::zftest::failure_count() == 0 ? 0 : 1;
}
