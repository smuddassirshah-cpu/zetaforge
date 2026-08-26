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

#include "radius.hpp"

namespace zetaforge {

class Ffix {
 public:
  using raw_t = i128;

  Ffix() : raw_(0), err_(0) {}

  static Ffix from_int(long v) {
    Ffix f;
    // The contract is on the RAW word, which is v << 64, so the admissible
    // range for v is |v| < 2^56. The previous guard compared v itself against
    // 2^120 and ignored negatives entirely, so it admitted values whose raw
    // word overflowed the 128-bit word outright. Same silent-wrap family as
    // the mul defect above (review B5).
    const raw_t lim = static_cast<raw_t>(1) << 56;
    const raw_t vv = static_cast<raw_t>(v);
    if (!(vv < lim && vv > -lim)) {
      throw std::overflow_error("ffix range");
    }
    f.raw_ = vv << 64;
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
    const u128 am = abs_u128(raw_);
    const u128 bm = abs_u128(o.raw_);
    if (!(am < (u128)1 << 120) || !(bm < (u128)1 << 120)) {
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
    const u128 top = (u128)ah * bh;
    u128 mid = (u128)ah * bl;
    mid += (u128)al * bh;
    const u128 low = (u128)al * bl;

    const uint64_t lost = static_cast<uint64_t>(low);

    // Overflow must be decided BEFORE the shift, not after it. Gate finding
    // (review B5): `res = top << 64` wraps modulo 2^128 whenever top >= 2^64,
    // and the range test below then inspected the wrapped value. With the
    // operand contract |raw| < 2^120 the high limbs satisfy ah, bh < 2^56, so
    // top can reach 2^112 and the shift discarded 49 bits in silence:
    // from_int(2^32).mul(from_int(2^32)) returned raw 0 with err 1 and no
    // throw, against this file's own "products beyond range throw rather than
    // wrap silently".
    //
    // The result is floor(M / 2^64) for M = |a|*|b|, so overflow means
    // M >= 2^191. Bit-length form: top = ah*bh is the top limb of M, and
    // top >= 2^63 implies res >= top*2^64 >= 2^127. Rejecting that first makes
    // the shift exact (top < 2^63 gives top<<64 < 2^127), after which the
    // assembled range test below is itself exact. mid < 2^121 and low >> 64 <
    // 2^64 under the same operand contract, so the sum cannot wrap either.
    if (!(top < (u128)1 << 63)) {
      throw std::overflow_error("ffix mul overflow");
    }
    u128 res = top << 64;
    res += mid;
    res += static_cast<u128>(low >> 64);
    if (!(res < (u128)1 << 127)) {
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

  static u128 abs_u128(raw_t v) {
    return v < 0 ? (u128)(-v) : (u128)v;
  }

  // Promote an existing error count to whole-word granularity against a
  // magnitude: conservative, monotone, never smaller than the input.
  static raw_t inflate_err(raw_t units, u128 magnitude) {
    const raw_t words = (raw_t)((magnitude >> 32) + 1);
    const raw_t scaled = units * (words + 1);
    return scaled > units ? scaled : units;
  }

  raw_t raw_;
  raw_t err_;
};

}  // namespace zetaforge
