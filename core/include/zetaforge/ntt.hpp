#pragma once

// Decision note: exact number-theoretic transform convolution over the
// NTT-friendly prime p = 998244353 = 119 * 2^23 + 1 with generator 3.
//
// Exactness: all arithmetic is modular on uint64_t; no rounding exists to
// track. Two entry points:
//   - convolve_cyclic(a, b): length-n cyclic convolution, n a power of two
//   - convolve_linear(a, b): full linear convolution, zero-padded internally
//             to the next power of two >= |a| + |b| - 1, then trimmed
// Non-power-of-two cyclic lengths throw rather than silently padding, so the
// padding decision stays at the caller's boundary where it belongs.

#include <cstddef>
#include <stdexcept>
#include <vector>

#include "radius.hpp"

namespace zetaforge {

namespace ntt_detail {
constexpr uint64_t kMod = 998244353ULL;  // 119 * 2^23 + 1
constexpr uint64_t kGen = 3ULL;
constexpr size_t kMaxLen = size_t{1} << 23;

inline uint64_t add_mod(uint64_t a, uint64_t b) {
  return a + b >= kMod ? a + b - kMod : a + b;
}

inline uint64_t sub_mod(uint64_t a, uint64_t b) {
  return a >= b ? a - b : a + kMod - b;
}

inline uint64_t mul_mod(uint64_t a, uint64_t b) {
  return static_cast<uint64_t>((static_cast<u128>(a) * b) % kMod);
}

inline uint64_t pow_mod(uint64_t base, uint64_t e) {
  uint64_t r = 1;
  while (e != 0) {
    if (e & 1) r = mul_mod(r, base);
    base = mul_mod(base, base);
    e >>= 1;
  }
  return r;
}

inline void transform(std::vector<uint64_t>& a, bool inverse) {
  const size_t n = a.size();
  for (size_t i = 1, j = 0; i < n; ++i) {
    size_t bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) std::swap(a[i], a[j]);
  }
  for (size_t len = 2; len <= n; len <<= 1) {
    uint64_t wlen = pow_mod(kGen, (kMod - 1) / len);
    if (inverse) {
      wlen = pow_mod(wlen, kMod - 2);  // modular inverse via Fermat
    }
    for (size_t i = 0; i < n; i += len) {
      uint64_t w = 1;
      const size_t half = len / 2;
      for (size_t k = 0; k < half; ++k) {
        const uint64_t u = a[i + k];
        const uint64_t v = mul_mod(a[i + k + half], w);
        a[i + k] = add_mod(u, v);
        a[i + k + half] = sub_mod(u, v);
        w = mul_mod(w, wlen);
      }
    }
  }
  if (inverse) {
    const uint64_t n_inv = pow_mod(static_cast<uint64_t>(n), kMod - 2);
    for (uint64_t& x : a) x = mul_mod(x, n_inv);
  }
}

}  // namespace ntt_detail

// Length-n cyclic convolution; both inputs must be length n, n a power of two.
inline std::vector<uint64_t> convolve_cyclic(const std::vector<uint64_t>& a,
                                             const std::vector<uint64_t>& b) {
  if (a.size() != b.size()) {
    throw std::invalid_argument("convolve_cyclic requires equal lengths");
  }
  const size_t n = a.size();
  if (n == 0) {
    return {};
  }
  if ((n & (n - 1)) != 0 || n > ntt_detail::kMaxLen) {
    throw std::invalid_argument("convolve_cyclic requires power-of-two length <= 2^23");
  }
  std::vector<uint64_t> fa(a);
  std::vector<uint64_t> fb(b);
  ntt_detail::transform(fa, false);
  ntt_detail::transform(fb, false);
  for (size_t i = 0; i < n; ++i) {
    fa[i] = ntt_detail::mul_mod(fa[i], fb[i]);
  }
  ntt_detail::transform(fa, true);
  return fa;
}

// Full linear convolution, result size |a| + |b| - 1.
inline std::vector<uint64_t> convolve_linear(const std::vector<uint64_t>& a,
                                             const std::vector<uint64_t>& b) {
  if (a.empty() || b.empty()) {
    return {};
  }
  const size_t out_len = a.size() + b.size() - 1;
  size_t n = 1;
  while (n < out_len) n <<= 1;
  if (n > ntt_detail::kMaxLen) {
    throw std::invalid_argument("convolve_linear too long for modulus");
  }
  std::vector<uint64_t> pa(n, 0);
  std::vector<uint64_t> pb(n, 0);
  for (size_t i = 0; i < a.size(); ++i) pa[i] = a[i] % ntt_detail::kMod;
  for (size_t i = 0; i < b.size(); ++i) pb[i] = b[i] % ntt_detail::kMod;
  std::vector<uint64_t> c = convolve_cyclic(pa, pb);
  c.resize(out_len);
  return c;
}

}  // namespace zetaforge
