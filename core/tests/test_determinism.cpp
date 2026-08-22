// Cross-configuration determinism probe (stage 2 attack item 8).
//
// Runs a fixed operation sequence over Ball, Ffix and NTT and prints
//   DETERMINISM_HASH <hex16>
// CI runs this binary in the Release and Debug matrix legs and requires the
// two hashes to be identical: -ffp-contract=off is load-bearing from this
// stage onward, and any compiler-configuration divergence in the numeric path
// shows up here first.

#include <cstdint>
#include <cstdio>
#include <vector>

#include "check.hpp"
#include "zetaforge/ball.hpp"
#include "zetaforge/ffix.hpp"
#include "zetaforge/ntt.hpp"

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
    const uint64_t hi = static_cast<uint64_t>(static_cast<unsigned __int128>(fx.raw()) >> 64);
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

  // Self-consistency: the sequence must produce a non-trivial digest.
  const uint64_t first = g_hash;
  ZF_CHECK(first != 0);
  ZF_CHECK(first != UINT64_C(0xCBF29CE484222325));

  std::printf("DETERMINISM_HASH %016llx\n",
              static_cast<unsigned long long>(first));
  return ::zftest::failure_count() == 0 ? 0 : 1;
}
