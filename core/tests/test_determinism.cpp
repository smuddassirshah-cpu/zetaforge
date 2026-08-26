// Cross-configuration determinism probe (stage 2 attack item 8).
//
// Runs a fixed operation sequence over Ball, Ffix, NTT, theta_certified and
// every CBall operation, and prints
//   DETERMINISM_HASH <hex16>
// CI runs this binary in the Release and Debug matrix legs and requires the
// two hashes to be identical: -ffp-contract=off is load-bearing from this
// stage onward, and any compiler-configuration divergence in the numeric path
// shows up here first.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "check.hpp"
#include "zetaforge/ball.hpp"
#include "zetaforge/cball.hpp"
#include "zetaforge/ffix.hpp"
#include "zetaforge/ntt.hpp"
#include "zetaforge/theta.hpp"

namespace {

uint64_t g_hash = UINT64_C(0xCBF29CE484222325);

void mix_bytes(const void* data, size_t len) {
  const auto* bytes = static_cast<const unsigned char*>(data);
  for (size_t i = 0; i < len; ++i) {
    g_hash ^= bytes[i];
    g_hash *= UINT64_C(0x100000001B3);
  }
}

void mix_double(double x) { mix_bytes(&x, sizeof(x)); }

// Mixes an mpfr value at full stored precision rather than through a double.
// A double conversion would hide any divergence below 53 bits, which is most
// of the range the certified path actually operates in.
void mix_mpfr(mpfr_srcptr v) {
  mpfr_exp_t exp = 0;
  char* str = mpfr_get_str(nullptr, &exp, 16, 0, v, MPFR_RNDN);
  if (str != nullptr) {
    mix_bytes(str, std::strlen(str));
    mpfr_free_str(str);
  }
  mix_bytes(&exp, sizeof(exp));
}

}  // namespace

int main() {
  using zetaforge::Ball;
  using zetaforge::Ffix;

  Ball acc = Ball::parse("1.41421356237309504880", 128);
  Ball cur = Ball::from_double(1.7e88, 128);
  for (int i = 0; i < 64; ++i) {
    cur = Ball::mul(cur, Ball::from_double(0.999, 128));
    acc = Ball::add(acc, Ball::scale(cur, 3));
    if (i % 8 == 0) {
      acc = Ball::sub(acc, Ball::parse("6.62607015e-34", 128));
    }
    mix_double(mpfr_get_d(acc.centre(), MPFR_RNDN));
    mix_double(acc.radius());
  }

  Ffix fx = Ffix::from_int(987654321);
  Ffix fy = Ffix::from_raw(static_cast<Ffix::raw_t>(0x123456789ABCDEFull));
  for (int i = 0; i < 32; ++i) {
    fx = fx.mul(fy).add(fy).sub(Ffix::from_int(i + 1));
    const uint64_t lo = static_cast<uint64_t>(fx.raw());
    const uint64_t hi = static_cast<uint64_t>(static_cast<zetaforge::u128>(fx.raw()) >> 64);
    mix_bytes(&lo, sizeof(lo));
    mix_bytes(&hi, sizeof(hi));
  }

  std::vector<uint64_t> va(256), vb(256);
  for (size_t i = 0; i < va.size(); ++i) {
    va[i] = static_cast<uint64_t>(i) * UINT64_C(7919) % 998244353ULL;
    vb[i] = static_cast<uint64_t>(i * i + 13) % 998244353ULL;
  }
  const std::vector<uint64_t> conv = zetaforge::convolve_linear(va, vb);
  uint64_t checksum = 0;
  for (size_t i = 0; i < conv.size(); ++i) {
    checksum = (checksum * UINT64_C(31) + conv[i]) % 998244353ULL;
  }
  mix_bytes(&checksum, sizeof(checksum));

  // ---- theta_certified (review finding C6) --------------------------------
  // The hash previously covered Ball, Ffix and NTT only, so the entire
  // certified theta path and the complex ball layer were outside the only
  // cross-configuration check the project runs. A contraction or reassociation
  // difference there would not have shown up anywhere.
  {
    const double heights[] = {200.0, 1000.0, 1.0e6, 3.0e12};
    const mpfr_prec_t precs[] = {128, 256};
    for (const double h : heights) {
      for (const mpfr_prec_t pr : precs) {
        const Ball th = zetaforge::theta_certified(h, pr);
        mix_mpfr(th.centre());
        mix_double(th.radius());
      }
    }
  }

  // ---- CBall: every operation ---------------------------------------------
  {
    using zetaforge::CBall;
    constexpr mpfr_prec_t P = 160;
    CBall a(P), b(P), c(P);
    mpfr_set_d(a.re, 3.25, MPFR_RNDN);
    mpfr_set_d(a.im, -1.5, MPFR_RNDN);
    a.rr = 0x1p-20;
    a.ri = 0x1p-21;
    mpfr_set_d(b.re, -0.75, MPFR_RNDN);
    mpfr_set_d(b.im, 2.125, MPFR_RNDN);
    b.rr = 0x1p-19;
    b.ri = 0x1p-22;

    mpfr_t cf;
    mpfr_init2(cf, P);
    mpfr_set_d(cf, 1.0 / 3.0, MPFR_RNDN);

    for (int i = 0; i < 24; ++i) {
      c = a;
      c.mul(b, P);           // mul
      c.add(a);              // add
      c.mul_real(cf, 0x1p-30, P);  // mul_real
      if ((i & 3) == 0) {
        c.negate();          // negate
      }
      mix_mpfr(c.re);
      mix_mpfr(c.im);
      mix_double(c.rr);
      mix_double(c.ri);
      a = c;
      if ((i & 7) == 7) {
        a.set_zero();
        mpfr_set_d(a.re, 3.25, MPFR_RNDN);
        mpfr_set_d(a.im, -1.5, MPFR_RNDN);
        a.rr = 0x1p-20;
        a.ri = 0x1p-21;
      }
    }
    mpfr_clear(cf);
  }

  // Self-consistency: the sequence must produce a non-trivial digest.
  const uint64_t first = g_hash;
  ZF_CHECK(first != 0);
  ZF_CHECK(first != UINT64_C(0xCBF29CE484222325));

  std::printf("DETERMINISM_HASH %016llx\n",
              static_cast<unsigned long long>(first));
  return ::zftest::failure_count() == 0 ? 0 : 1;
}
