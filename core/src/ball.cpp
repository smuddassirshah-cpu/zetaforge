#include "zetaforge/ball.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

#include "zetaforge/radius.hpp"

namespace zetaforge {

namespace {
constexpr double kInf = std::numeric_limits<double>::infinity();
}

void Ball::init(mpfr_prec_t prec_bits) {
  if (prec_bits < 53) {
    throw std::invalid_argument("ball precision below double resolution");
  }
  mpfr_init2(centre_, prec_bits);
  mpfr_set_zero(centre_, 0);
  radius_ = 0.0;
}

Ball::Ball(mpfr_prec_t prec_bits) {
  init(prec_bits);
}

Ball::Ball(const Ball& other) : radius_(other.radius_) {
  mpfr_init2(centre_, mpfr_get_prec(other.centre_));
  mpfr_set(centre_, other.centre_, MPFR_RNDN);
}

Ball::Ball(Ball&& other) noexcept : radius_(other.radius_) {
  mpfr_init2(centre_, mpfr_get_prec(other.centre_));
  mpfr_swap(centre_, other.centre_);
}

Ball& Ball::operator=(const Ball& other) {
  if (this != &other) {
    if (mpfr_get_prec(centre_) != mpfr_get_prec(other.centre_)) {
      mpfr_set_prec(centre_, mpfr_get_prec(other.centre_));
    }
    mpfr_set(centre_, other.centre_, MPFR_RNDN);
    radius_ = other.radius_;
  }
  return *this;
}

Ball& Ball::operator=(Ball&& other) noexcept {
  if (this != &other) {
    mpfr_swap(centre_, other.centre_);
    radius_ = std::exchange(other.radius_, 0.0);
  }
  return *this;
}

Ball::~Ball() {
  mpfr_clear(centre_);
}

Ball Ball::from_double(double x, mpfr_prec_t prec_bits) {
  if (!std::isfinite(x)) {
    throw std::invalid_argument("ball scalar must be finite");
  }
  Ball out(prec_bits);
  mpfr_set_d(out.centre_, x, MPFR_RNDN);
  out.radius_ = 0.0;
  return out;
}

// Half an ulp of c at c's own precision, rounded outward into a double.
//
// Gate finding (review B4). The previous implementation took
// half_ulp_bound(mpfr_get_d(centre)), a bound on the DOUBLE image rather than
// on the stored mpfr value, and it failed at both ends of the range:
//   parse("1e-320", 128) -> the double image is subnormal, the halved span
//     ties-to-even to zero, and the ball claimed EXACTNESS about a value whose
//     true representation error is ~3.7e-359.
//   parse("1e400", 128)  -> the double image is infinite, the old code
//     substituted denorm_min, and the ball claimed a radius ~684 orders of
//     magnitude below the true error of ~1.5e361.
// Both are enclosure violations in an approved stage 2 public API.
//
// Derivation: mpfr stores |c| in [2^(e-1), 2^e) at precision p, so ulp(c) is
// 2^(e-p) and round-to-nearest gives |true - c| <= 2^(e-p-1). That exponent is
// read from the ROUNDED centre, so a value carried up into the next binade by
// rounding is bounded by its own larger ulp. Powers of two are exact in a
// double wherever they are representable, so the conversion adds no error;
// outside that range the policy is explicit rather than silent.
double Ball::representation_radius(mpfr_srcptr c) {
  if (mpfr_zero_p(c)) {
    return 0.0;  // zero is exactly representable at every precision
  }
  const long e = static_cast<long>(mpfr_get_exp(c));
  const long p = static_cast<long>(mpfr_get_prec(c));
  const long k = e - p - 1;  // half ulp = 2^k
  if (k > 1023) {
    return kInf;  // beyond double range: unknown_at_precision, never a number
  }
  if (k < -1074) {
    return std::numeric_limits<double>::denorm_min();  // never false-exact
  }
  return std::ldexp(1.0, static_cast<int>(k));
}

Ball Ball::parse(const std::string& decimal, mpfr_prec_t prec_bits) {
  Ball out(prec_bits);
  const int ret = mpfr_set_str(out.centre_, decimal.c_str(), 10, MPFR_RNDN);
  if (ret != 0) {
    throw std::invalid_argument("unparsable ball literal");
  }
  out.radius_ = representation_radius(out.centre_);
  return out;
}

double Ball::abs_centre_ceiling(mpfr_srcptr c) {
  mpfr_t abs_tmp;
  mpfr_init2(abs_tmp, mpfr_get_prec(c));
  mpfr_abs(abs_tmp, c, MPFR_RNDN);
  // Directed conversion: result is guaranteed >= the true magnitude.
  double u = mpfr_get_d(abs_tmp, MPFR_RNDU);
  mpfr_clear(abs_tmp);
  if (u == 0.0 && !mpfr_zero_p(c)) {
    // Sub-magnitude magnitudes must not vanish: a huge radius times a tiny
    // centre is still a contribution.
    u = std::numeric_limits<double>::denorm_min();
  }
  return u;
}

double Ball::finish_radius(double terms, mpfr_srcptr rounded_centre) {
  const double with_rounding =
      up_add(terms, half_ulp_bound(mpfr_get_d(rounded_centre, MPFR_RNDN)));
  return inflate(with_rounding);
}

Ball Ball::from_centre_and_radius(mpfr_srcptr centre, double radius) {
  if (!std::isfinite(radius) || radius < 0.0) {
    throw std::invalid_argument("certified radius must be finite non-negative");
  }
  if (!mpfr_number_p(centre)) {
    throw std::invalid_argument("certified centre must be finite");
  }
  Ball out(mpfr_get_prec(centre));
  mpfr_set(out.centre_, centre, MPFR_RNDN);
  out.radius_ = radius;
  return out;
}

Ball Ball::add(const Ball& a, const Ball& b) {
  Ball out(std::max(a.precision(), b.precision()));
  mpfr_add(out.centre_, a.centre_, b.centre_, MPFR_RNDN);
  out.radius_ = finish_radius(up_add(a.radius_, b.radius_), out.centre_);
  return out;
}

Ball Ball::sub(const Ball& a, const Ball& b) {
  Ball out(std::max(a.precision(), b.precision()));
  mpfr_sub(out.centre_, a.centre_, b.centre_, MPFR_RNDN);
  out.radius_ = finish_radius(up_add(a.radius_, b.radius_), out.centre_);
  return out;
}

Ball Ball::mul(const Ball& a, const Ball& b) {
  Ball out(std::max(a.precision(), b.precision()));
  mpfr_mul(out.centre_, a.centre_, b.centre_, MPFR_RNDN);

  const double ua = abs_centre_ceiling(a.centre_);
  const double ub = abs_centre_ceiling(b.centre_);
  double terms = up_mul(a.radius_, ub);
  terms = up_add(terms, up_mul(b.radius_, ua));
  terms = up_add(terms, up_mul(a.radius_, b.radius_));
  out.radius_ = finish_radius(terms, out.centre_);
  return out;
}

Ball Ball::scale(const Ball& a, long n) {
  const double nd = static_cast<double>(n);
  if (static_cast<long>(nd) != n) {
    throw std::invalid_argument("scale factor not exact in double");
  }
  Ball out(a.precision());
  mpfr_mul_si(out.centre_, a.centre_, n, MPFR_RNDN);
  double terms = up_mul(a.radius_, std::fabs(nd));
  out.radius_ = finish_radius(terms, out.centre_);
  return out;
}

bool Ball::contains_zero() const {
  if (!(radius_ < kInf)) {
    return true;  // infinite radius encloses everything
  }
  mpfr_t r;
  mpfr_init2(r, 53);
  mpfr_set_d(r, radius_, MPFR_RNDN);
  const int cmp = mpfr_cmp_abs(centre_, r);
  mpfr_clear(r);
  return cmp <= 0;
}

bool Ball::needs_escalation(double max_rel_width) const {
  if (unknown_at_precision()) {
    return true;
  }
  if (!std::isfinite(radius_)) {  // NaN defence
    return true;
  }
  const double mag = std::fabs(mpfr_get_d(centre_, MPFR_RNDN));
  const double denom = mag > 0.0 ? mag : std::numeric_limits<double>::denorm_min();
  return radius_ / denom > max_rel_width;
}

void Ball::widen_radius(double extra) {
  if (!std::isfinite(extra) || extra < 0.0) {
    throw std::invalid_argument("widen_radius needs finite non-negative extra");
  }
  radius_ = up_add(radius_, extra);
}

std::string Ball::centre_decimal(size_t digits) const {
  mpfr_exp_t exp = 0;
  char* str = mpfr_get_str(nullptr, &exp, 10, digits, centre_, MPFR_RNDN);
  if (str == nullptr) {
    return {};
  }
  std::string mantissa(str);
  mpfr_free_str(str);
  if (mantissa.empty()) {
    return mantissa;
  }
  if (mantissa.front() == '-') {
    mantissa.insert(1, "0.");
  } else {
    mantissa.insert(0, "0.");
  }
  return mantissa + "e" + std::to_string(exp);
}

}  // namespace zetaforge
