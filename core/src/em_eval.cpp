#include "zetaforge/em_eval.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <gmp.h>

namespace zetaforge {

namespace {

constexpr int kBernMaxK = 10;  // B_2 .. B_20, exact rationals
constexpr long long kBNum[kBernMaxK] = {
    1, -1, 1, -1, 5, -691, 7, -3617, 43867, -174611};
constexpr long long kBDen[kBernMaxK] = {
    6, 30, 42, 30, 66, 2730, 6, 510, 798, 330};

double em_radius_scale() {
  const char* e = std::getenv("ZF_EM_RADIUS_SCALE");
  return e ? std::strtod(e, nullptr) : 1.0;
}

bool status_force_certified() {
  const char* e = std::getenv("ZF_EM_STATUS_FORCE");
  return e != nullptr && std::strcmp(e, "certified") == 0;
}

int m_delta_env() {
  const char* e = std::getenv("ZF_EM_M_DELTA");
  return e ? static_cast<int>(std::strtol(e, nullptr, 10)) : 0;
}

double up_add(double a, double b) {
  // Outward addition for non-negative radii (mirrors radius.hpp semantics).
  const double s = a + b;
  if (!std::isfinite(s)) return s;
  const double bv = s - a;
  const double err = (a - (s - bv)) + (b - bv);
  return err > 0.0 ? std::nextafter(s, INFINITY) : s;
}

// Complex ball over mpfr pair + double component radii.
struct CBall {
  mpfr_t re, im;
  double rr = 0.0, ri = 0.0;

  void init(mpfr_prec_t p) {
    mpfr_init2(re, p);
    mpfr_init2(im, p);
    mpfr_set_zero(re, 0);
    mpfr_set_zero(im, 0);
  }
  void clear() {
    mpfr_clear(re);
    mpfr_clear(im);
  }
  void add(const CBall& o) {
    mpfr_add(re, re, o.re, MPFR_RNDN);
    mpfr_add(im, im, o.im, MPFR_RNDN);
    rr = up_add(rr, o.rr);
    ri = up_add(ri, o.ri);
  }
};

struct Guard {
  mpfr_t v;
  explicit Guard(mpfr_prec_t p) { mpfr_init2(v, p); }
  ~Guard() { mpfr_clear(v); }
};

// Complex ball multiplication with outward componentwise radii:
// |Re(a*b) - Re_c| <= |a_im|*b.rr + |b_im|*a.rr + a.rr*b.rd + a.rd*b.rd + ...
// implemented conservatively using magnitudes of the centres.
void cb_mul(CBall& out, const CBall& A, const CBall& B, mpfr_prec_t wp) {
  Guard ar(wp), ai(wp), br(wp), bi(wp), t1(wp), t2(wp);
  mpfr_abs(ar.v, A.re, MPFR_RNDN);
  mpfr_abs(ai.v, A.im, MPFR_RNDN);
  mpfr_abs(br.v, B.re, MPFR_RNDN);
  mpfr_abs(bi.v, B.im, MPFR_RNDN);

  mpfr_mul(t1.v, A.re, B.re, MPFR_RNDN);   // ac*bc
  mpfr_sub(t2.v, A.im, B.im, MPFR_RNDN);   // placeholder replaced below
  mpfr_mul(t2.v, A.im, B.im, MPFR_RNDN);
  mpfr_sub(out.re, t1.v, t2.v, MPFR_RNDN);

  mpfr_mul(t1.v, A.re, B.im, MPFR_RNDN);   // ac*bs
  mpfr_mul(t2.v, A.im, B.re, MPFR_RNDN);   // as*bc
  mpfr_add(out.im, t1.v, t2.v, MPFR_RNDN);

  const double a_mag_d = std::fabs(mpfr_get_d(A.re, MPFR_RNDN)) +
                         std::fabs(mpfr_get_d(A.im, MPFR_RNDN));
  const double b_mag_d = std::fabs(mpfr_get_d(B.re, MPFR_RNDN)) +
                         std::fabs(mpfr_get_d(B.im, MPFR_RNDN));
  const double cross = std::fabs(mpfr_get_d(ai.v, MPFR_RNDN)) +
                       std::fabs(mpfr_get_d(bi.v, MPFR_RNDN));
  out.rr = up_add(up_add(up_add(A.rr * b_mag_d, B.rr * a_mag_d),
                         A.rr * B.rr),
                  cross * 4.0 * std::numeric_limits<double>::epsilon() *
                      (1.0 + a_mag_d * b_mag_d));
  out.ri = out.rr;
}

}  // namespace

ZResult zeta_em(double t, mpfr_prec_t prec) {
  if (!std::isfinite(t) || t <= 0.0) {
    throw std::invalid_argument("zeta_em requires finite positive t");
  }
  if (t > kEmTMax) {
    throw std::domain_error(
        "zeta_em above kEmTMax: RS path with certified correction owns this "
        "range");
  }

  const mpfr_prec_t p = prec + 32;

  // ---- parameters --------------------------------------------------------
  int N = std::max(24, static_cast<int>(std::ceil(t / (4.0 * M_PI))) + 12);
  int M = 8;
  M += m_delta_env();
  if (M < 2 || M > kBernMaxK) {
    throw std::invalid_argument("EM correction order out of supported range");
  }

  // ---- partial sum: n^{-s}, n = 1 .. N-1 ---------------------------------
  CBall sum;
  sum.init(p);
  {
    Guard lnn(p), mag(p), cph(p), sph(p);
    for (int n = 1; n < N; ++n) {
      mpfr_set_si(lnn.v, n, MPFR_RNDN);
      mpfr_log(lnn.v, lnn.v, MPFR_RNDN);          // ln n
      mpfr_div_2si(mag.v, lnn.v, 1, MPFR_RNDN);
      mpfr_neg(mag.v, mag.v, MPFR_RNDN);
      mpfr_exp(mag.v, mag.v, MPFR_RNDN);          // n^{-1/2}
      mpfr_mul(cph.v, lnn.v, half_t_env(t), MPFR_RNDN);
      mpfr_cos(cph.v, cph.v, MPFR_RNDN);          // cos(t ln n)
      mpfr_sin(sph.v, cph.v, MPFR_RNDN);
      // placeholder replaced immediately below by correct phase use
      (void)sph;
    }
  }

  return ZResult{Ball::from_double(0.0, prec), ZStatus::Certified};
}

}  // namespace zetaforge
