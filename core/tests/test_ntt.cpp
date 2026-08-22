#include <cstdint>
#include <stdexcept>
#include <vector>

#include "check.hpp"
#include "zetaforge/ntt.hpp"

using zetaforge::convolve_cyclic;
using zetaforge::convolve_linear;

namespace {

std::vector<uint64_t> schoolbook(const std::vector<uint64_t>& a,
                                 const std::vector<uint64_t>& b, size_t out_len) {
  std::vector<unsigned __int128> acc(out_len, 0);
  for (size_t i = 0; i < a.size(); ++i) {
    for (size_t j = 0; j < b.size(); ++j) {
      acc[i + j] += static_cast<unsigned __int128>(a[i]) * b[j];
    }
  }
  std::vector<uint64_t> out(out_len);
  for (size_t i = 0; i < out_len; ++i) out[i] = static_cast<uint64_t>(acc[i] % 998244353ULL);
  return out;
}

}  // namespace

int main() {
  auto& rng = ::zftest::rng();

  // Known linear case.
  {
    const std::vector<uint64_t> c = convolve_linear({1, 2, 3}, {1, 1});
    ZF_CHECK(c.size() == 4);
    ZF_CHECK(c[0] == 1 && c[1] == 3 && c[2] == 5 && c[3] == 3);
  }

  // Randomised linear cross-check vs schoolbook across boundary lengths.
  for (size_t n : {size_t{1}, size_t{2}, size_t{3}, size_t{7}, size_t{64},
                   size_t{255}, size_t{256}}) {
    std::vector<uint64_t> a(n), b(n);
    for (size_t i = 0; i < n; ++i) {
      a[i] = rng.next() % 998244353ULL;
      b[i] = rng.next() % 998244353ULL;
    }
    const std::vector<uint64_t> fast = convolve_linear(a, b);
    const std::vector<uint64_t> slow = schoolbook(a, b, a.size() + b.size() - 1);
    ZF_CHECK(fast == slow);
  }

  // Cyclic convolution matches wrapped linear.
  {
    const size_t n = 128;
    std::vector<uint64_t> a(n), b(n);
    for (size_t i = 0; i < n; ++i) {
      a[i] = rng.next() % 998244353ULL;
      b[i] = rng.next() % 998244353ULL;
    }
    const std::vector<uint64_t> cyc = convolve_cyclic(a, b);
    const std::vector<uint64_t> lin = schoolbook(a, b, n);  // truncated wrap: recompute
    // wrap the linear result manually
    std::vector<unsigned __int128> acc(n, 0);
    for (size_t i = 0; i < n; ++i)
      for (size_t j = 0; j < n; ++j) acc[(i + j) % n] += static_cast<unsigned __int128>(a[i]) * b[j];
    bool ok = true;
    for (size_t i = 0; i < n; ++i)
      if (cyc[i] != static_cast<uint64_t>(acc[i] % 998244353ULL)) ok = false;
    ZF_CHECK(ok);
    (void)lin;
  }

  // Round-trip: convolving with a unit impulse at index 0 is the identity.
  {
    const size_t n = 512;
    std::vector<uint64_t> a(n), delta(n, 0);
    for (size_t i = 0; i < n; ++i) a[i] = rng.next() % 998244353ULL;
    delta[0] = 1;
    const std::vector<uint64_t> back = convolve_cyclic(a, delta);
    ZF_CHECK(back == a);

    // Impulse at index k is a cyclic shift by k.
    std::vector<uint64_t> shift(n, 0);
    shift[137 % n] = 1;
    const std::vector<uint64_t> shifted = convolve_cyclic(a, shift);
    bool shifted_ok = true;
    for (size_t i = 0; i < n; ++i) {
      if (shifted[i] != a[(i + n - (137 % n)) % n]) shifted_ok = false;
    }
    ZF_CHECK(shifted_ok);
  }

  // Rejections.
  bool threw = false;
  try {
    auto r = convolve_cyclic(std::vector<uint64_t>(100, 1), std::vector<uint64_t>(100, 1));
    (void)r;
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  ZF_CHECK(threw);

  threw = false;
  try {
    auto r = convolve_cyclic(std::vector<uint64_t>(16, 1), std::vector<uint64_t>(32, 1));
    (void)r;
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  ZF_CHECK(threw);

  return ::zftest::failure_count() == 0 ? 0 : 1;
}
