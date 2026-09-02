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
//
// Error arithmetic is composed in UNSIGNED 128-bit words with SATURATING add
// and multiply, never in the signed raw word (rev 6, found by the sanitiser
// leg). The composition previously ran in signed __int128 and overflowed:
// UndefinedBehaviorSanitizer caught a signed overflow on the ea * words term
// after 32 chained multiplications in test_determinism. Signed overflow is
// undefined, and the observed behaviour is a wrap, which makes the tracked
// bound SMALLER than the truth. inflate_err also detected its own overflow
// after the fact and fell back to the unscaled input, which is the same
// post-hoc pattern review finding B5 rejected in the value path, in the error
// path instead. An error at kErrMax means the value carries no usable bound
// and the caller must escalate, exactly as an infinite Ball radius does; it is
// never a small number standing in for a large one.

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
    if (err_units < 0) {
      throw std::invalid_argument("ffix error count must be non-negative");
    }
    f.raw_ = r;
    f.err_ = err_units;
    return f;
  }

  raw_t raw() const { return raw_; }

  // Tracked error bound in raw units (1 unit = 2^-64). Conservative by design.
  raw_t err() const { return err_; }

  // True when the bound has saturated: the value carries no usable error
  // information and a caller relying on it must escalate rather than read err().
  bool err_saturated() const { return static_cast<u128>(err_) >= kErrMax; }

  // Exact operations.
  Ffix add(const Ffix& o) const { return make_checked(raw_ + o.raw_, sum_err(o)); }
  Ffix sub(const Ffix& o) const { return make_checked(raw_ - o.raw_, sum_err(o)); }
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
    // slips through untracked. All quantities are raw-unit counts, composed in
    // u128 with saturating operations so the bound can only ever grow.
    const u128 ea = err_ == 0 ? 0 : inflate_err(to_u(err_), am);
    const u128 eb = o.err_ == 0 ? 0 : inflate_err(to_u(o.err_), bm);
    u128 e = sat_add(ea, eb);
    // ea * |b| in absolute terms maps to ea * ceil_words(bm) raw units here:
    // both operands share the Q64.64 scale, so an error of ea raw units times
    // magnitude bm contributes roughly ea*bm >> 32 units after promotion.
    e = sat_add(e, sat_mul(ea, (bm >> 32) + 1));
    e = sat_add(e, sat_mul(eb, (am >> 32) + 1));
    e = sat_add(e, lost != 0 ? 1 : 0);
    e = sat_add(e, 1);  // binary-op never-exact policy, mirrors Ball

    return make_checked(out_raw, static_cast<raw_t>(e));
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

  // Largest error count representable in the signed raw word. Saturation
  // ceiling for the whole error path; see err_saturated().
  static constexpr u128 kErrMax = (((u128)1) << 127) - 1;

  // A negative err_ cannot exist (from_raw refuses it; every producer clamps
  // at kErrMax), so this maps corruption, not input. Corruption becomes the
  // POISON MARKER, never a zero bound: mapping it to 0 would be the silent
  // under-report this file's decision note condemns (rev 7 verification).
  static u128 to_u(raw_t v) { return v < 0 ? kErrMax : (u128)v; }

  static u128 sat_add(u128 a, u128 b) {
    const u128 s = a + b;          // unsigned: wrap is defined, and detectable
    return (s < a || s > kErrMax) ? kErrMax : s;
  }

  static u128 sat_mul(u128 a, u128 b) {
    if (a == 0 || b == 0) {
      return 0;
    }
    // Decided BEFORE the multiplication, not inspected after it.
    if (a > kErrMax / b) {
      return kErrMax;
    }
    return a * b;
  }

  raw_t sum_err(const Ffix& o) const {
    return static_cast<raw_t>(sat_add(to_u(err_), to_u(o.err_)));
  }

  // Promote an existing error count to whole-word granularity against a
  // magnitude: monotone by construction, since words + 1 >= 1 and sat_mul
  // saturates high. The defensive 'scaled > units ? scaled : units' ternary
  // that stood here was DEAD in the fixed code and was the exact shape of
  // the pre-fix fallback that made the B1 wrap silent; removed at rev 7 so
  // the shape cannot be copied back into a live path (verification finding).
  static u128 inflate_err(u128 units, u128 magnitude) {
    const u128 words = (magnitude >> 32) + 1;
    return sat_mul(units, words + 1);
  }

  raw_t raw_;
  raw_t err_;
};

}  // namespace zetaforge
