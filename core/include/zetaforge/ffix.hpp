#pragma once

// Decision note: fixed-point FFT-friendly type. Q64.64 signed representation
// on a 128-bit raw word; value = raw * 2^-64.
//
// Error tracking policy (stage 2 attack list, item 6): the tracked bound is
// composed with DELIBERATE over-estimates (magnitudes rounded up to 2^32-word
// granularity) and can therefore never under-report the true accumulated
// truncation error. The property suite measures the actual error against an
// exact reference and asserts bound >= actual; shrinking the safety margins
// must make that test fail.
//
// Dynamic range contract: |raw| < 2^120 enforced at construction and before
// multiplication. Products beyond range throw rather than wrap silently.

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace zetaforge {

class Ffix {
 public:
  using raw_t = __int128;

  Ffix() : raw_(0), err_(0) {}

  static Ffix from_int(long v) {
    Ffix f;
    if (v > 0 && static_cast<raw_t>(v) > (static_cast<raw_t>(1) << 120)) {
      throw std::overflow_error("ffix range");
    }
    f.raw_ = static_cast<raw_t>(v) << 64;
    return f;
  }

  static Ffix from_raw(raw_t r, raw_t err_units = 0) {
    Ffix f;
    if (!in_range(r)) {
      throw std::overflow_error("ffix range");
    }
    f.raw_ = r;
    f.err_ = err_units;
    return f;
  }

  raw_t raw() const { return raw_; }

  // Tracked error bound in raw units (1 unit = 2^-64). Conservative by design.
  raw_t err() const { return err_; }

  // Exact operations.
  Ffix add(const Ffix& o) const { return make_checked(raw_ + o.raw_, err_ + o.err_); }
  Ffix sub(const Ffix& o) const { return make_checked(raw_ - o.raw_, err_ + o.err_); }
  Ffix negate() const { return make_checked(-raw_, err_); }

  // Multiplication with explicit truncation tracking.
  Ffix mul(const Ffix& o) const {
    const bool neg = (raw_ < 0) != (o.raw_ < 0);
    const unsigned __int128 am = abs_u128(raw_);
    const unsigned __int128 bm = abs_u128(o.raw_);
    if (!(am < (unsigned __int128)1 << 120) || !(bm < (unsigned __int128)1 << 120)) {
      throw std::overflow_error("ffix mul range");
    }

    const uint64_t ah = static_cast<uint64_t>(am >> 64);
    const uint64_t al = static_cast<uint64_t>(am);
    const uint64_t bh = static_cast<uint64_t>(bm >> 64);
    const uint64_t bl = static_cast<uint64_t>(bm);

    // Magnitude product M = A*B as three limbs of base 2^64:
    //   top  = ah*bh
    //   mid  = ah*bl + al*bh
    //   low  = al*bl
    // Q64.64 result raw units = floor(M / 2^64)
    //   = top<<64 + mid + (low>>64); dropped part = low & mask64 (< 1 unit).
    const unsigned __int128 top = (unsigned __int128)ah * bh;
    unsigned __int128 mid = (unsigned __int128)ah * bl;
    mid += (unsigned __int128)al * bh;
    const unsigned __int128 low = (unsigned __int128)al * bl;

    const uint64_t lost = static_cast<uint64_t>(low);
    unsigned __int128 res = top << 64;
    res += mid;
    res += static_cast<unsigned __int128>(low >> 64);
    if (!(res < (unsigned __int128)1 << 127)) {
      throw std::overflow_error("ffix mul overflow");
    }
    if (!(res < (unsigned __int128)1 << 127)) {
      throw std::overflow_error("ffix mul overflow");
    }

    const raw_t mag = static_cast<raw_t>(res);
    const raw_t out_raw = neg ? -mag : mag;

    // Error composition, deliberately inflated: operand magnitudes are
    // promoted to whole 2^32 words before multiplication so no partial ulp
    // slips through untracked. All quantities are raw-unit counts.
    const raw_t ea = err_ == 0 ? 0 : inflate_err(err_, am);
    const raw_t eb = o.err_ == 0 ? 0 : inflate_err(o.err_, bm);
    raw_t e = ea + eb;
    // ea * |b| in absolute terms maps to ea * ceil_words(bm) raw units here:
    // both operands share the Q64.64 scale, so an error of ea raw units times
    // magnitude bm contributes roughly ea*bm >> 32 units after promotion.
    e += ea == 0 ? 0 : (ea * (raw_t)((bm >> 32) + 1));
    e += eb == 0 ? 0 : (eb * (raw_t)((am >> 32) + 1));
    e += (lost != 0 ? 1 : 0);
    e += 1;  // binary-op never-exact policy, mirrors Ball

    return make_checked(out_raw, e);
  }

 private:
  Ffix(raw_t r, raw_t e) : raw_(r), err_(e) {}

  static bool in_range(raw_t r) {
    const raw_t lim = static_cast<raw_t>(1) << 120;
    return r < lim && r > -lim;
  }

  static Ffix make_checked(raw_t r, raw_t e) {
    if (!in_range(r)) {
      throw std::overflow_error("ffix range");
    }
    return Ffix(r, e);
  }

  static unsigned __int128 abs_u128(raw_t v) {
    return v < 0 ? (unsigned __int128)(-v) : (unsigned __int128)v;
  }

  // Promote an existing error count to whole-word granularity against a
  // magnitude: conservative, monotone, never smaller than the input.
  static raw_t inflate_err(raw_t units, unsigned __int128 magnitude) {
    const raw_t words = (raw_t)((magnitude >> 32) + 1);
    const raw_t scaled = units * (words + 1);
    return scaled > units ? scaled : units;
  }

  raw_t raw_;
  raw_t err_;
};

}  // namespace zetaforge
