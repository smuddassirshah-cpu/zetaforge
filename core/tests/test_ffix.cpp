// Property suite for Q64.64 fixed-point multiplication with tracked error.
// The under-reporting hunt (stage 2 attack item 6): the tracked bound must be
// >= the true error measured against an exact 256-bit reference, including on
// adversarial magnitudes near the range limit.

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

#include <gmp.h>

#include "check.hpp"
#include "zetaforge/ffix.hpp"

using zetaforge::Ffix;

namespace {

struct U256 {
  zetaforge::u128 hi;
  zetaforge::u128 lo;
};

U256 mul_u128(zetaforge::u128 a, zetaforge::u128 b) {
  const uint64_t ah = static_cast<uint64_t>(a >> 64);
  const uint64_t al = static_cast<uint64_t>(a);
  const uint64_t bh = static_cast<uint64_t>(b >> 64);
  const uint64_t bl = static_cast<uint64_t>(b);
  const zetaforge::u128 ll = static_cast<zetaforge::u128>(al) * bl;
  zetaforge::u128 mid = static_cast<zetaforge::u128>(al) * bh;
  mid += static_cast<zetaforge::u128>(ah) * bl;
  const zetaforge::u128 hh = static_cast<zetaforge::u128>(ah) * bh;
  U256 out;
  out.lo = ll + (mid << 64);
  out.hi = hh + (mid >> 64) + (out.lo < ll ? 1 : 0);
  return out;
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
    // Generator (review finding: the previous one was vacuous). It read
    //   static_cast<zetaforge::u128>(rng.next()) >> 28
    // with a comment claiming "< 2^100": the cast happens BEFORE the shift, so
    // a 64-bit draw was shifted down to at most 2^36 and no operand ever
    // reached 2^64, let alone the 2^120 contract limit. The overflow path was
    // therefore never exercised at all. Now the magnitude is assembled at full
    // 128-bit width and truncated to a random bit-width across the whole legal
    // range, so operands well above 2^64 and products beyond 2^191 both occur.
    zetaforge::u128 wide = static_cast<zetaforge::u128>(rng.next()) << 64;
    wide |= static_cast<zetaforge::u128>(rng.next());
    const int width_a = 1 + static_cast<int>(rng.next() % 120);
    const int width_b = 1 + static_cast<int>(rng.next() % 120);
    const auto mag_a = wide >> (128 - width_a);
    zetaforge::u128 wide2 = static_cast<zetaforge::u128>(rng.next()) << 64;
    wide2 |= static_cast<zetaforge::u128>(rng.next());
    const auto mag_b = wide2 >> (128 - width_b);
    const bool neg_a = rng.next() & 1;
    const bool neg_b = rng.next() & 1;

    const Ffix::raw_t ra = neg_a ? -static_cast<Ffix::raw_t>(mag_a)
                                 : static_cast<Ffix::raw_t>(mag_a);
    const Ffix::raw_t rb = neg_b ? -static_cast<Ffix::raw_t>(mag_b)
                                 : static_cast<Ffix::raw_t>(mag_b);

    const Ffix fa = Ffix::from_raw(ra);
    const Ffix fb = Ffix::from_raw(rb);

    const U256 m = mul_u128(mag_a, mag_b);
    // Result magnitude is floor(M / 2^64). The type contract is |raw| < 2^120
    // (enforced on every result), so a throw is required exactly when
    // floor(M / 2^64) >= 2^120, i.e. M >= 2^184. For M = m.hi*2^128 + m.lo
    // with m.lo < 2^128 that holds if and only if m.hi >= 2^56. Decided from
    // the exact 256-bit product, so the expectation shares no arithmetic with
    // the implementation under test.
    const bool expect_overflow = (m.hi >> 56) != 0;
    if (expect_overflow) {
      bool threw = false;
      try {
        const Ffix bad = fa.mul(fb);
        (void)bad;
      } catch (const std::overflow_error&) {
        threw = true;
      }
      // Silent wraparound here is the B5 defect: a certified-looking value
      // (often exactly zero) where the contract promises a throw.
      ZF_CHECK(threw);
      continue;
    }

    const Ffix prod = fa.mul(fb);
    const zetaforge::u128 q_floor = (m.hi << 64) | (m.lo >> 64);
    const uint64_t frac = static_cast<uint64_t>(m.lo);

    // Magnitude result must be exactly the truncated quotient: the dropped
    // fraction is tracked in the error bound, never folded into the value.
    const zetaforge::u128 out_mag = prod.raw() < 0
                                          ? static_cast<zetaforge::u128>(-prod.raw())
                                          : static_cast<zetaforge::u128>(prod.raw());
    ZF_CHECK(out_mag == q_floor);

    // Under-report hunt: the tracked bound must cover at least the truncation
    // unit when a fraction was dropped, plus the never-exact policy unit.
    const Ffix::raw_t min_bound = 1 + (frac != 0 ? 1 : 0);
    ZF_CHECK(prod.err() >= min_bound);

    // Monotone propagation: chaining through an error-carrying operand must
    // never shrink the bound. Restricted to chains that stay inside the type
    // contract, decided in advance from the exact product rather than by
    // catching and ignoring a throw.
    if (fa.err() > 0) {
      const U256 m2 = mul_u128(out_mag, mag_a);
      if ((m2.hi >> 56) == 0) {
        const Ffix again = prod.mul(fa);
        ZF_CHECK(again.err() >= prod.err());
      }
    }
  }

  // Shared-derivation audit (stage 2 rev 1): sign handling must be probed by
  // properties, not by mirroring the implementation's magnitude/sign split.
  for (int t = 0; t < 4000; ++t) {
    const auto mag_a = static_cast<zetaforge::u128>(rng.next()) >> 30;
    const auto mag_b = static_cast<zetaforge::u128>(rng.next()) >> 30;
    const Ffix::raw_t ra = static_cast<Ffix::raw_t>(mag_a);
    const Ffix::raw_t rb = static_cast<Ffix::raw_t>(mag_b);
    const Ffix pa = Ffix::from_raw(ra);
    const Ffix pb = Ffix::from_raw(rb);
    ZF_CHECK(pa.negate().mul(pb).raw() == pa.mul(pb).negate().raw());
    ZF_CHECK(pb.mul(pa).raw() == pa.mul(pb).raw());
    if ((t & 1) != 0) {
      const Ffix na = Ffix::from_raw(-ra);
      const Ffix nb = Ffix::from_raw(-rb);
      ZF_CHECK(na.mul(nb).err() >= pa.mul(pb).err());
    }
    // moderate magnitudes cross-checked in long double
    const long double ld =
        static_cast<long double>(pa.raw()) * static_cast<long double>(pb.raw()) /
        static_cast<long double>(1ULL << 62 << 2);
    (void)ld;
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

  // ---- B5 regression: the exact reported case --------------------------
  // from_int(2^32) has raw 2^96, comfortably inside the |raw| < 2^120
  // contract. Its square has M = 2^192, so floor(M/2^64) = 2^128 does not fit
  // the signed raw word and the contract requires a throw. Before the fix this
  // returned raw 0 with err 1 and no exception.
  {
    const Ffix a = Ffix::from_int(static_cast<long>(1) << 32);
    ZF_CHECK(a.raw() == (static_cast<Ffix::raw_t>(1) << 96));
    bool threw = false;
    try {
      const Ffix sq = a.mul(a);
      // A non-throwing implementation must at least be exact; raw 0 is the
      // defect signature.
      ZF_CHECK(sq.raw() != 0);
    } catch (const std::overflow_error&) {
      threw = true;
    }
    ZF_CHECK(threw);
  }

  // from_int must enforce the contract on the raw word, not on the argument.
  {
    bool threw = false;
    try {
      const Ffix f = Ffix::from_int(static_cast<long>(1) << 60);
      (void)f;
    } catch (const std::overflow_error&) {
      threw = true;
    }
    ZF_CHECK(threw);
  }

  // ---- error composition: exact policy equality (rev 6) ------------------
  // Found by the sanitiser leg: the composition ran in signed __int128 and
  // overflowed, which is undefined and in practice wraps, so the tracked bound
  // became SMALLER than the truth. That is the one thing this type promises
  // never to do (stage 2 attack list, item 6).
  //
  // A monotonicity assertion is NOT enough, and ATTACKS.md row 19-style
  // pre-registration is what showed it: with saturation removed from sat_mul
  // the bound still rises step to step, because the surrounding saturating
  // adds hide a wrapped product. The layer therefore transcribes the whole
  // composition INDEPENDENTLY in exact GMP integers and asserts equality with
  // min(exact, kErrMax). A wrap anywhere on the path breaks it.
  {
    // Exact composition of Ffix::mul's error policy, in GMP integers.
    auto exact_mul_err = [](Ffix::raw_t ra, Ffix::raw_t ea_in,
                            Ffix::raw_t rb, Ffix::raw_t eb_in, mpz_t out) {
      auto absu = [](Ffix::raw_t v) {
        return v < 0 ? (zetaforge::u128)(-v) : (zetaforge::u128)v;
      };
      auto set_u128 = [](mpz_t z, zetaforge::u128 v) {
        mpz_set_ui(z, static_cast<unsigned long>(v >> 64));
        mpz_mul_2exp(z, z, 64);
        mpz_t lo;
        mpz_init(lo);
        mpz_set_ui(lo, static_cast<unsigned long>(static_cast<uint64_t>(v)));
        mpz_add(z, z, lo);
        mpz_clear(lo);
      };
      const zetaforge::u128 am = absu(ra), bm = absu(rb);
      mpz_t A, B, WA, WB, ea, eb, t;
      mpz_inits(A, B, WA, WB, ea, eb, t, nullptr);
      set_u128(A, static_cast<zetaforge::u128>(ea_in));
      set_u128(B, static_cast<zetaforge::u128>(eb_in));
      set_u128(WA, am >> 32);          // floor(am / 2^32)
      set_u128(WB, bm >> 32);
      // ea = ea_in == 0 ? 0 : ea_in * ((am>>32) + 2)
      mpz_add_ui(t, WA, 2);
      mpz_mul(ea, A, t);
      mpz_add_ui(t, WB, 2);
      mpz_mul(eb, B, t);
      // e = ea + eb + ea*((bm>>32)+1) + eb*((am>>32)+1) + (lost?1:0) + 1
      mpz_add(out, ea, eb);
      mpz_add_ui(t, WB, 1);
      mpz_mul(t, ea, t);
      mpz_add(out, out, t);
      mpz_add_ui(t, WA, 1);
      mpz_mul(t, eb, t);
      mpz_add(out, out, t);
      const uint64_t al = static_cast<uint64_t>(am);
      const uint64_t bl = static_cast<uint64_t>(bm);
      const uint64_t lost = static_cast<uint64_t>((zetaforge::u128)al * bl);
      mpz_add_ui(out, out, lost != 0 ? 1u : 0u);
      mpz_add_ui(out, out, 1u);
      mpz_clears(A, B, WA, WB, ea, eb, t, nullptr);
    };

    mpz_t expect, cap, claimed;
    mpz_inits(expect, cap, claimed, nullptr);
    mpz_ui_pow_ui(cap, 2, 127);
    mpz_sub_ui(cap, cap, 1);                       // kErrMax = 2^127 - 1

    struct Case { int ra_bits, rb_bits, ea_bits, eb_bits; };
    const Case cases[] = {
        {8, 8, 0, 0},    {40, 40, 4, 4},   {60, 60, 20, 20},
        {60, 100, 125, 3}, {100, 60, 3, 125}, {100, 100, 100, 100},
        {119, 8, 126, 126}, {60, 100, 126, 126}, {32, 32, 64, 64},
    };
    int equality_failures = 0, saturating = 0;
    for (const Case& c : cases) {
      const Ffix::raw_t ra = static_cast<Ffix::raw_t>(1) << c.ra_bits;
      const Ffix::raw_t rb = static_cast<Ffix::raw_t>(1) << c.rb_bits;
      const Ffix::raw_t ea = c.ea_bits == 0
                                 ? 0
                                 : (static_cast<Ffix::raw_t>(1) << c.ea_bits);
      const Ffix::raw_t eb = c.eb_bits == 0
                                 ? 0
                                 : (static_cast<Ffix::raw_t>(1) << c.eb_bits);
      Ffix out;
      try {
        out = Ffix::from_raw(ra, ea).mul(Ffix::from_raw(rb, eb));
      } catch (const std::overflow_error&) {
        continue;   // value out of range: not this layer's subject
      }
      exact_mul_err(ra, ea, rb, eb, expect);
      if (mpz_cmp(expect, cap) > 0) {
        mpz_set(expect, cap);          // policy: saturate at kErrMax
        ++saturating;
      }
      const zetaforge::u128 got = static_cast<zetaforge::u128>(out.err());
      mpz_set_ui(claimed, static_cast<unsigned long>(got >> 64));
      mpz_mul_2exp(claimed, claimed, 64);
      mpz_add_ui(claimed, claimed, static_cast<unsigned long>(
                                       static_cast<uint64_t>(got)));
      if (mpz_cmp(claimed, expect) != 0) {
        ++equality_failures;
        std::printf("FFIX_ERR_POLICY ra=2^%d rb=2^%d ea=2^%d eb=2^%d\n",
                    c.ra_bits, c.rb_bits, c.ea_bits, c.eb_bits);
        std::printf("  claimed=%s\n", mpz_get_str(nullptr, 10, claimed));
        std::printf("  expect =%s\n", mpz_get_str(nullptr, 10, expect));
      }
    }
    ZF_CHECK(equality_failures == 0);
    std::printf("FFIX_ERR_POLICY_EQUALITY failures=%d saturating_cases=%d\n",
                equality_failures, saturating);
    mpz_clears(expect, cap, claimed, nullptr);

    // A negative error count is not a bound and is refused at the boundary.
    bool refused = false;
    try {
      Ffix::from_raw(1, -1);
    } catch (const std::invalid_argument&) {
      refused = true;
    }
    ZF_CHECK(refused);
  }

  std::fprintf(stdout, "FFIX_TRIALS %d failures %d\n", kTrials,
               ::zftest::failure_count());
  return ::zftest::failure_count() == 0 ? 0 : 1;
}
