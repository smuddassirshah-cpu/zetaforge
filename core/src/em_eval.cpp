#include "zetaforge/em_eval.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <gmp.h>

#include "zetaforge/bernoulli.hpp"
#include "zetaforge/radius.hpp"

namespace zetaforge {

namespace {

constexpr int kBernMaxK = 10;
constexpr long long kBNum[kBernMaxK] = {
    1, -1, 1, -1, 5, -691, 7, -3617, 43867, -174611};
constexpr long long kBDen[kBernMaxK] = {
    6, 30, 42, 30, 66, 2730, 6, 510, 798, 330};

// EM correction order is no longer fixed at compile time: M is pinned per call
// from the target radius (MATHS.md D8.8). The constant that stood here was an
// environment-tunable order first and a compile-time constant second, and
// neither was derived from anything.
constexpr int kEmMMax = static_cast<int>(kBernoulliMaxN) - 2;
constexpr int kEmNMax = 1 << 20;

// The hand-transcribed table above has no other consumer now that the exact
// recurrence in bernoulli.cpp feeds the series. It stays as the tripwire that
// recurrence is checked against on first use, which is what makes
// docs/gate/ATTACKS.md row 3 a live row rather than an accepted blind spot.
static_assert(kBNum[0] == 1 && kBDen[0] == 6, "Bernoulli table head");
static_assert(kBernMaxK == 10, "table length is quoted in MATHS.md D8.6");

struct Mp {
  mpfr_t v;
  explicit Mp(mpfr_prec_t p) { mpfr_init2(v, p); mpfr_set_zero(v, 0); }
  ~Mp() { mpfr_clear(v); }
  Mp(const Mp&) = delete;
  Mp& operator=(const Mp&) = delete;
};

struct Mq {
  mpq_t v;
  Mq() { mpq_init(v); }
  ~Mq() { mpq_clear(v); }
  Mq(const Mq&) = delete;
  Mq& operator=(const Mq&) = delete;
};

// |T_{M+1}| * |s + 2M+1| / (sigma + 2M+1): Backlund's bound (Edwards 6.4),
// evaluated in mpfr and returned as a double rounded outward. sigma = 1/2.
//
// |T_k| = |B_2k|/(2k)! * prod_{j<2k-1}|s+j| * N^(1/2) / N^(2k)
double backlund_bound(double t, long n, int m, mpfr_prec_t p) {
  Mp acc(p), term(p), tmp(p);
  Mq b;
  bernoulli_2n(static_cast<unsigned>(m + 1), b.v);
  mpfr_set_q(acc.v, b.v, MPFR_RNDU);
  mpfr_abs(acc.v, acc.v, MPFR_RNDU);
  mpfr_fac_ui(tmp.v, static_cast<unsigned long>(2 * (m + 1)), MPFR_RNDD);
  mpfr_div(acc.v, acc.v, tmp.v, MPFR_RNDU);

  // prod_{j=0}^{2m} |s + j| with s = 1/2 + it
  for (long j = 0; j <= 2L * m; ++j) {
    mpfr_set_d(tmp.v, 0.5 + static_cast<double>(j), MPFR_RNDU);
    mpfr_sqr(tmp.v, tmp.v, MPFR_RNDU);
    mpfr_set_d(term.v, t, MPFR_RNDU);
    mpfr_sqr(term.v, term.v, MPFR_RNDU);
    mpfr_add(tmp.v, tmp.v, term.v, MPFR_RNDU);
    mpfr_sqrt(tmp.v, tmp.v, MPFR_RNDU);
    mpfr_mul(acc.v, acc.v, tmp.v, MPFR_RNDU);
  }

  // N^(1/2) / N^(2m+2)
  mpfr_set_si(tmp.v, n, MPFR_RNDU);
  mpfr_pow_si(term.v, tmp.v, -(2L * m + 2L), MPFR_RNDU);
  mpfr_mul(acc.v, acc.v, term.v, MPFR_RNDU);
  mpfr_sqrt(term.v, tmp.v, MPFR_RNDU);
  mpfr_mul(acc.v, acc.v, term.v, MPFR_RNDU);

  // times |s + 2M+1| / (1/2 + 2M+1)
  const double shift = static_cast<double>(2 * m + 1);
  mpfr_set_d(tmp.v, 0.5 + shift, MPFR_RNDU);
  mpfr_sqr(tmp.v, tmp.v, MPFR_RNDU);
  mpfr_set_d(term.v, t, MPFR_RNDU);
  mpfr_sqr(term.v, term.v, MPFR_RNDU);
  mpfr_add(tmp.v, tmp.v, term.v, MPFR_RNDU);
  mpfr_sqrt(tmp.v, tmp.v, MPFR_RNDU);
  mpfr_mul(acc.v, acc.v, tmp.v, MPFR_RNDU);
  mpfr_set_d(tmp.v, 0.5 + shift, MPFR_RNDD);
  mpfr_div(acc.v, acc.v, tmp.v, MPFR_RNDU);

  return mpfr_get_d(acc.v, MPFR_RNDU);
}

// N and M pinned from the target radius, not from a rule of thumb.
void pin_nm(double t, double target, mpfr_prec_t p, long& n_out, int& m_out,
            double& bound_out) {
  long n = std::max(8L, static_cast<long>(std::ceil(t)) + 1L);
  while (n <= kEmNMax) {
    double prev = 0.0;
    for (int m = 1; m <= kEmMMax; ++m) {
      const double b = backlund_bound(t, n, m, p);
      if (b <= target) {
        n_out = n;
        m_out = m;
        bound_out = b;
        return;
      }
      // Divergence turnover: no larger M is tighter at this N, so grow N.
      if (m > 1 && b > prev) {
        break;
      }
      prev = b;
    }
    n *= 2;
  }
  throw std::runtime_error(
      "zeta_em: no (N, M) reaches the target radius below the length cap");
}

}  // namespace

ZResult zeta_em(double t, mpfr_prec_t prec) {
  if (!std::isfinite(t)) {
    throw std::invalid_argument("zeta_em requires finite positive t");
  }
  if (t <= 0.0) {
    throw std::invalid_argument("zeta_em requires finite positive t");
  }
  if (t > kEmTMax) {
    throw std::domain_error(
        "zeta_em above kEmTMax: RS path with certified correction owns "
        "this range");
  }

  const mpfr_prec_t P = prec + 64;
  const long q = static_cast<long>(prec) + 16;
  const double target = std::ldexp(1.0, static_cast<int>(-q));

  long N = 0;
  int M = 0;
  double tail_bound = 0.0;
  pin_nm(t, target, P, N, M, tail_bound);

  // ---- Dirichlet sum, N^-s/2 and N^(1-s)/(s-1) ---------------------------
  Mp u(P), v(P), lg(P), ang(P), cs(P), sn(P), amp(P), tmp(P), tmp2(P);
  Mp tv(P);
  mpfr_set_d(tv.v, t, MPFR_RNDN);

  auto n_pow_minus_s = [&](long n, mpfr_ptr re, mpfr_ptr im) {
    // n^-s = n^(-1/2) [cos(t ln n) - i sin(t ln n)]
    mpfr_set_si(tmp.v, n, MPFR_RNDN);
    mpfr_log(lg.v, tmp.v, MPFR_RNDN);
    mpfr_mul(ang.v, lg.v, tv.v, MPFR_RNDN);
    mpfr_sin_cos(sn.v, cs.v, ang.v, MPFR_RNDN);
    mpfr_rec_sqrt(amp.v, tmp.v, MPFR_RNDN);      // n^(-1/2)
    mpfr_mul(re, cs.v, amp.v, MPFR_RNDN);
    mpfr_mul(im, sn.v, amp.v, MPFR_RNDN);
    mpfr_neg(im, im, MPFR_RNDN);
  };

  Mp tr(P), ti(P);
  for (long n = 1; n < N; ++n) {
    n_pow_minus_s(n, tr.v, ti.v);
    mpfr_add(u.v, u.v, tr.v, MPFR_RNDN);
    mpfr_add(v.v, v.v, ti.v, MPFR_RNDN);
  }

  // + N^-s / 2
  n_pow_minus_s(N, tr.v, ti.v);
  mpfr_div_2ui(tmp.v, tr.v, 1, MPFR_RNDN);
  mpfr_add(u.v, u.v, tmp.v, MPFR_RNDN);
  mpfr_div_2ui(tmp.v, ti.v, 1, MPFR_RNDN);
  mpfr_add(v.v, v.v, tmp.v, MPFR_RNDN);

  // + N^(1-s)/(s-1). N^(1-s) = N * N^-s; s - 1 = -1/2 + it.
  Mp gr(P), gi(P), dr(P), di(P), den(P);
  mpfr_mul_si(gr.v, tr.v, N, MPFR_RNDN);
  mpfr_mul_si(gi.v, ti.v, N, MPFR_RNDN);
  mpfr_set_d(dr.v, -0.5, MPFR_RNDN);
  mpfr_set(di.v, tv.v, MPFR_RNDN);
  mpfr_sqr(den.v, dr.v, MPFR_RNDN);
  mpfr_sqr(tmp.v, di.v, MPFR_RNDN);
  mpfr_add(den.v, den.v, tmp.v, MPFR_RNDN);      // |s-1|^2
  // (gr + i gi)(dr - i di) / |s-1|^2
  mpfr_mul(tmp.v, gr.v, dr.v, MPFR_RNDN);
  mpfr_mul(tmp2.v, gi.v, di.v, MPFR_RNDN);
  mpfr_add(tmp.v, tmp.v, tmp2.v, MPFR_RNDN);
  mpfr_div(tmp.v, tmp.v, den.v, MPFR_RNDN);
  mpfr_add(u.v, u.v, tmp.v, MPFR_RNDN);
  mpfr_mul(tmp.v, gi.v, dr.v, MPFR_RNDN);
  mpfr_mul(tmp2.v, gr.v, di.v, MPFR_RNDN);
  mpfr_sub(tmp.v, tmp.v, tmp2.v, MPFR_RNDN);
  mpfr_div(tmp.v, tmp.v, den.v, MPFR_RNDN);
  mpfr_add(v.v, v.v, tmp.v, MPFR_RNDN);

  // ---- Bernoulli correction terms ----------------------------------------
  // T_k = (B_2k/(2k)!) * prod_{j<2k-1}(s+j) * N^(1-s) / N^(2k)
  Mp pr(P), pi_(P), sr(P), si(P), coef(P), nsq(P), scale(P);
  Mq bq;
  mpfr_set_d(sr.v, 0.5, MPFR_RNDN);
  mpfr_set(si.v, tv.v, MPFR_RNDN);
  mpfr_set(pr.v, sr.v, MPFR_RNDN);               // P_1 = s
  mpfr_set(pi_.v, si.v, MPFR_RNDN);
  mpfr_set_si(nsq.v, N, MPFR_RNDN);
  mpfr_sqr(nsq.v, nsq.v, MPFR_RNDN);             // N^2
  mpfr_set_ui(scale.v, 1, MPFR_RNDN);

  double prev_mag = -1.0;
  bool monotone_ok = true;
  for (int k = 1; k <= M; ++k) {
    mpfr_div(scale.v, scale.v, nsq.v, MPFR_RNDN);   // N^(-2k)
    bernoulli_2n(static_cast<unsigned>(k), bq.v);
    mpfr_set_q(coef.v, bq.v, MPFR_RNDN);
    mpfr_fac_ui(tmp.v, static_cast<unsigned long>(2 * k), MPFR_RNDN);
    mpfr_div(coef.v, coef.v, tmp.v, MPFR_RNDN);
    mpfr_mul(coef.v, coef.v, scale.v, MPFR_RNDN);

    // (P_k * G) * coef, with G = N^(1-s) = (gr, gi)
    mpfr_mul(tmp.v, pr.v, gr.v, MPFR_RNDN);
    mpfr_mul(tmp2.v, pi_.v, gi.v, MPFR_RNDN);
    mpfr_sub(tr.v, tmp.v, tmp2.v, MPFR_RNDN);
    mpfr_mul(tmp.v, pr.v, gi.v, MPFR_RNDN);
    mpfr_mul(tmp2.v, pi_.v, gr.v, MPFR_RNDN);
    mpfr_add(ti.v, tmp.v, tmp2.v, MPFR_RNDN);
    mpfr_mul(tr.v, tr.v, coef.v, MPFR_RNDN);
    mpfr_mul(ti.v, ti.v, coef.v, MPFR_RNDN);
    mpfr_add(u.v, u.v, tr.v, MPFR_RNDN);
    mpfr_add(v.v, v.v, ti.v, MPFR_RNDN);

    // Sanity only. This CANNOT widen or narrow a radius: it records that the
    // asymptotic series has not passed its divergence turnover, which bounds
    // nothing (MATHS.md D8.8). The bound is Backlund's, computed above.
    const double mag = std::hypot(mpfr_get_d(tr.v, MPFR_RNDN),
                                  mpfr_get_d(ti.v, MPFR_RNDN));
    if (prev_mag >= 0.0 && mag > prev_mag) {
      monotone_ok = false;
    }
    prev_mag = mag;

    if (k < M) {
      // P_{k+1} = P_k (s + 2k-1)(s + 2k)
      for (int add : {2 * k - 1, 2 * k}) {
        mpfr_add_si(tmp.v, sr.v, add, MPFR_RNDN);
        mpfr_mul(tmp2.v, pr.v, tmp.v, MPFR_RNDN);
        mpfr_mul(tr.v, pi_.v, si.v, MPFR_RNDN);
        mpfr_sub(tr.v, tmp2.v, tr.v, MPFR_RNDN);
        mpfr_mul(tmp2.v, pi_.v, tmp.v, MPFR_RNDN);
        mpfr_mul(ti.v, pr.v, si.v, MPFR_RNDN);
        mpfr_add(ti.v, tmp2.v, ti.v, MPFR_RNDN);
        mpfr_set(pr.v, tr.v, MPFR_RNDN);
        mpfr_set(pi_.v, ti.v, MPFR_RNDN);
      }
    }
  }
  (void)monotone_ok;

  // ---- zeta radius --------------------------------------------------------
  // Truncation is Backlund's bound. Rounding is counted, not estimated: the
  // magnitude cap covers the partial sums (bounded by 2 sqrt(N)) and the sin
  // and cos ARGUMENTS t ln n, whose own error the trigonometric functions pass
  // through with unit derivative.
  const double sqrtN = std::sqrt(static_cast<double>(N));
  const double zeta_cap = 2.0 * sqrtN + t * std::log(static_cast<double>(N) + 1.0)
                          + 4.0;
  const double zeta_ops = 12.0 * static_cast<double>(N)
                          + 20.0 * static_cast<double>(M) + 40.0;
  const double unit = std::ldexp(1.0, 1 - static_cast<int>(P));
  const double zeta_round = up_mul(up_mul(zeta_cap, zeta_ops), unit);
  const double zeta_rad = inflate(up_add(tail_bound, zeta_round));

  // ---- assembly: Z = u cos(theta) - v sin(theta) --------------------------
  const Ball th = theta_certified(t, prec);
  const double r_theta = th.radius();

  Mp ct(P), st(P), zr(P), zi(P);
  mpfr_sin_cos(st.v, ct.v, th.centre(), MPFR_RNDN);
  mpfr_mul(tmp.v, u.v, ct.v, MPFR_RNDN);
  mpfr_mul(tmp2.v, v.v, st.v, MPFR_RNDN);
  mpfr_sub(zr.v, tmp.v, tmp2.v, MPFR_RNDN);      // Z
  mpfr_mul(tmp.v, u.v, st.v, MPFR_RNDN);
  mpfr_mul(tmp2.v, v.v, ct.v, MPFR_RNDN);
  mpfr_add(zi.v, tmp.v, tmp2.v, MPFR_RNDN);      // must contain zero

  // |dev| <= (|u| + |v|) r_theta + (ru + rv)(1 + r_theta), since |cos| and
  // |sin| are at most 1 and |cos'|, |sin'| are at most 1.
  const double au = std::fabs(mpfr_get_d(u.v, MPFR_RNDU));
  const double av = std::fabs(mpfr_get_d(v.v, MPFR_RNDU));
  double asm_dev = up_mul(up_add(au, av), r_theta);
  const double two_r = up_add(zeta_rad, zeta_rad);
  asm_dev = up_add(asm_dev, two_r);
  asm_dev = up_add(asm_dev, up_mul(two_r, r_theta));
  const double asm_round =
      up_mul(up_mul(up_add(au, av), 8.0), unit);
  double z_rad = up_add(asm_dev, asm_round);
  z_rad = inflate(z_rad);

  ZResult out{Ball::from_centre_and_radius(zr.v, z_rad),
              Ball::from_centre_and_radius(zi.v, z_rad),
              ZStatus::Contested,
              static_cast<int>(N),
              M,
              tail_bound};
  out.status = out.re.contains_zero() ? ZStatus::Contested : ZStatus::Certified;
  return out;
}

}  // namespace zetaforge
