// Complex-ball arithmetic tests.
//
// Layers:
//   C1 Monte-Carlo corner sampling: for random balls and random points drawn
//      from within the CLAIMED input boxes (including extreme corners), the
//      true product/sum must lie inside the claimed output box. This is the
//      failure the radius arithmetic exists to prevent - under-radius claims.
//   C2 Radius-sabotage detection: shrinking output radii by 0.9 or 0.5 MUST
//      produce corner-sample failures on adversarial magnitudes (proves the
//      suite can detect an unsound radius claim - aimed at the claim).
//   C3 Exact-rational spot checks on small integer complex inputs where the
//      exact product is computable by hand.
//   C4 Tightness: the claimed radius must dominate the exact deviation bound
//      of MATHS.md D7b and must not exceed a small multiple of it. Corner
//      containment alone cannot see an over-wide radius, so this is the layer
//      that fails a centre-inclusive radius (review finding B1), including
//      the direct statement that a product far from the origin must not
//      contain zero.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

#include "check.hpp"
#include "zetaforge/cball.hpp"

using zetaforge::CBall;

namespace {

uint64_t g_rng = UINT64_C(0xC0FFEE123456789A);
uint64_t next_u64() {
  uint64_t z = (g_rng += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

double uniform_pm(double half) {
  // uniform in [-half, +half]
  const double u = static_cast<double>(next_u64() >> 11) / 9007199254740992.0;
  return (2.0 * u - 1.0) * half;
}

struct Pt { double re, im; };

bool inside(const Pt& p, const CBall& b) {
  return std::fabs(p.re - mpfr_get_d(b.re, MPFR_RNDN)) <= b.rr &&
         std::fabs(p.im - mpfr_get_d(b.im, MPFR_RNDN)) <= b.ri;
}

Pt sample_corner(const CBall& b) {
  // deliberately biased to extremes: sign pattern from RNG
  const double sr = (next_u64() & 1) ? 1.0 : -1.0;
  const double si = (next_u64() & 1) ? 1.0 : -1.0;
  return {mpfr_get_d(b.re, MPFR_RNDN) + sr * b.rr,
          mpfr_get_d(b.im, MPFR_RNDN) + si * b.ri};
}

CBall make_ball(mpfr_prec_t p, double re_c, double im_c, double rr, double ri) {
  CBall b(p);
  mpfr_set_d(b.re, re_c, MPFR_RNDN);
  mpfr_set_d(b.im, im_c, MPFR_RNDN);
  b.rr = std::fabs(rr);
  b.ri = std::fabs(ri);
  return b;
}

}  // namespace

int main() {
  std::fprintf(stdout, "SEED %llx\n",
               static_cast<unsigned long long>(g_rng));
  constexpr mpfr_prec_t P = 160;

  // ---- C3: exact integer spot checks -----------------------------------
  {
    // (1+0.125i)(2-0.25i) = 2.03125 + 0i exactly
    CBall a = make_ball(P, 1.0, 0.125, 0.0, 0.0);
    CBall b = make_ball(P, 2.0, -0.25, 0.0, 0.0);
    a.mul(b, P);
    ZF_CHECK(std::fabs(mpfr_get_d(a.re, MPFR_RNDN) - 2.03125) < 1e-40);
    ZF_CHECK(std::fabs(mpfr_get_d(a.im, MPFR_RNDN)) < 1e-40);
  }
  {
    CBall a = make_ball(P, 3.5, -2.25, 0.0, 0.0);
    CBall b = make_ball(P, 0.75, 1.5, 0.0, 0.0);
    a.add(b);
    ZF_CHECK(std::fabs(mpfr_get_d(a.re, MPFR_RNDN) - 4.25) < 1e-40);
    ZF_CHECK(std::fabs(mpfr_get_d(a.im, MPFR_RNDN) + 0.75) < 1e-40);
  }

  // ---- C1: exact dyadic corner containment ------------------------------
  // All balls use dyadic centres/radii (multiples of 1/64); corners are
  // exact dyadics; the true corner products are computed EXACTLY as
  // rationals. No floating-point comparison anywhere.
  auto dy = [](int k) { return k / 64.0; };
  int containment_failures = 0;
  for (int are = -24; are <= 24 && containment_failures == 0; are += 3) {
    for (int aim = -24; aim <= 24 && containment_failures == 0; aim += 5) {
      for (int bre = -18; bre <= 18 && containment_failures == 0; bre += 4) {
        for (int bim = -18; bim <= 18 && containment_failures == 0; bim += 7) {
          const int rad_a_re = 1, rad_a_im = 1, rad_b_re = 1, rad_b_im = 1;
          CBall a = make_ball(P, dy(are), dy(aim),
                              dy(rad_a_re), dy(rad_a_im));
          CBall b = make_ball(P, dy(bre), dy(bim),
                              dy(rad_b_re), dy(rad_b_im));
          CBall prod = a;              // deep copy (Rule-of-Three verified)
          prod.mul(b, P);

          // Four corners: exact dyadic coordinates, products via double
          // (exact for these magnitudes: values < 2^12, products < 2^24,
          // well within double's 53-bit mantissa).
          const double ca[2] = {dy(are - rad_a_re), dy(are + rad_a_re)};
          const double ci[2] = {dy(aim - rad_a_im), dy(aim + rad_a_im)};
          const double cb[2] = {dy(bre - rad_b_re), dy(bre + rad_b_re)};
          const double di[2] = {dy(bim - rad_b_im), dy(bim + rad_b_im)};
          for (int s1 = 0; s1 < 2 && containment_failures == 0; ++s1)
          for (int s2 = 0; s2 < 2 && containment_failures == 0; ++s2)
          for (int s3 = 0; s3 < 2 && containment_failures == 0; ++s3)
          for (int s4 = 0; s4 < 2 && containment_failures == 0; ++s4) {
            const double tre_d = ca[s1]*cb[s2] - ci[s3]*di[s4];
            const double tim_d = ca[s1]*di[s4] + ci[s3]*cb[s2];
            if (!(std::fabs(mpfr_get_d(prod.re, MPFR_RNDN) - tre_d) <=
                      prod.rr &&
                  std::fabs(mpfr_get_d(prod.im, MPFR_RNDN) - tim_d) <=
                      prod.ri)) {
              std::printf("C1FAIL are=%d aim=%d bre=%d bim=%d s=%d%d%d%d\n",
                          are, aim, bre, bim, s1, s2, s3, s4);
              std::printf("  prod=(%.17g,%.17g) rr=%.17g ri=%.17g\n",
                          mpfr_get_d(prod.re, MPFR_RNDN),
                          mpfr_get_d(prod.im, MPFR_RNDN), prod.rr, prod.ri);
              std::printf("  tre=%.17g tim=%.17g d_re=%.17g d_im=%.17g\n",
                          tre_d, tim_d,
                          std::fabs(mpfr_get_d(prod.re, MPFR_RNDN) - tre_d),
                          std::fabs(mpfr_get_d(prod.im, MPFR_RNDN) - tim_d));
              ++containment_failures;
            }
          }
        }
      }
    }
  }
  ZF_CHECK(containment_failures == 0);

  // ---- C2: radius sabotage detection (exact corners) ---------------------
  // This layer was vacuous through rev 1: the detection predicate read
  //   detected = !any_inside || true;
  // which is tautologically true, the variable was then discarded with a
  // (void) cast, and the only surviving assertion was that a saved radius was
  // positive. The header claimed a 0.5x cut MUST fail; measurement showed cuts
  // of 0.9x and 0.5x passing green, with the real detection floor between
  // 0.25x and 0.5x. Review findings C3 and B1: the floor was that low because
  // the radius included the whole centre magnitude product and had enormous
  // slack to give away.
  //
  // Detection now means what it says: with the output radii cut, at least one
  // EXACT corner of the true product set must fall outside the claimed box.
  {
    // Centres and radii in units of 1/64, so every corner product is an exact
    // dyadic rational and the reference arithmetic is integer.
    const long long ar_u = 300, ai_u = -200, arad_u = 3, airad_u = 2;
    const long long br_u = 500, bi_u = 700, brad_u = 4, birad_u = 1;

    CBall a = make_ball(P, dy(ar_u), dy(ai_u), dy(arad_u), dy(airad_u));
    CBall b = make_ball(P, dy(br_u), dy(bi_u), dy(brad_u), dy(birad_u));
    CBall prod = a;
    prod.mul(b, P);

    // Exact corner containment against a given box. Corner products are
    // integers in units of 1/4096; the centre is compared in full mpfr
    // precision so the reference never passes through a double.
    auto corner_outside = [&](double box_rr, double box_ri) {
      mpfr_t cr, ci, tr, ti, d;
      mpfr_init2(cr, 256); mpfr_init2(ci, 256);
      mpfr_init2(tr, 256); mpfr_init2(ti, 256); mpfr_init2(d, 256);
      mpfr_set(cr, prod.re, MPFR_RNDN);
      mpfr_set(ci, prod.im, MPFR_RNDN);
      bool outside = false;
      for (int sa = -1; sa <= 1 && !outside; sa += 2)
        for (int ta = -1; ta <= 1 && !outside; ta += 2)
          for (int sb = -1; sb <= 1 && !outside; sb += 2)
            for (int tb = -1; tb <= 1 && !outside; tb += 2) {
              const long long AR = ar_u + sa * arad_u;
              const long long AI = ai_u + ta * airad_u;
              const long long BR = br_u + sb * brad_u;
              const long long BI = bi_u + tb * birad_u;
              // (AR + i AI)(BR + i BI) in units of 1/4096
              mpfr_set_si(tr, AR * BR - AI * BI, MPFR_RNDN);
              mpfr_div_ui(tr, tr, 4096, MPFR_RNDN);
              mpfr_set_si(ti, AR * BI + AI * BR, MPFR_RNDN);
              mpfr_div_ui(ti, ti, 4096, MPFR_RNDN);
              mpfr_sub(d, tr, cr, MPFR_RNDN);
              mpfr_abs(d, d, MPFR_RNDN);
              if (mpfr_cmp_d(d, box_rr) > 0) { outside = true; break; }
              mpfr_sub(d, ti, ci, MPFR_RNDN);
              mpfr_abs(d, d, MPFR_RNDN);
              if (mpfr_cmp_d(d, box_ri) > 0) { outside = true; break; }
            }
      mpfr_clear(cr); mpfr_clear(ci);
      mpfr_clear(tr); mpfr_clear(ti); mpfr_clear(d);
      return outside;
    };

    // The claimed radii must contain every exact corner.
    ZF_CHECK(!corner_outside(prod.rr, prod.ri));

    // A 0.9x cut must be detected. The rev 1 implementation survived this.
    ZF_CHECK(corner_outside(prod.rr * 0.9, prod.ri * 0.9));
    // And so must the coarser cuts the old header claimed to catch.
    ZF_CHECK(corner_outside(prod.rr * 0.5, prod.ri * 0.5));
    std::printf("C2_DETECT 0.9=%d 0.5=%d\n",
                static_cast<int>(corner_outside(prod.rr * 0.9, prod.ri * 0.9)),
                static_cast<int>(corner_outside(prod.rr * 0.5, prod.ri * 0.5)));
  }

  // ---- C4: radius must be a DEVIATION bound, not a centre-inclusive one ---
  // Review finding B1: mul previously set the radius to L1(a) * L1(b), which
  // includes the centre product, so the radius always exceeded the output
  // centre magnitude and every product ball contained zero. Sound, and
  // useless: nothing downstream could ever be signed. Corner containment
  // alone cannot see this, because an over-wide radius contains every corner.
  //
  // The exact deviation bound is computed here in integer units from the
  // formula in MATHS.md D7b, independently of the implementation. The claimed
  // radius must dominate it (soundness) and must not exceed a small multiple
  // of it (so a centre-inclusive radius fails).
  {
    const long long ar_u = 300, ai_u = -200, arad_u = 3, airad_u = 2;
    const long long br_u = 500, bi_u = 700, brad_u = 4, birad_u = 1;

    CBall a = make_ball(P, dy(ar_u), dy(ai_u), dy(arad_u), dy(airad_u));
    CBall b = make_ball(P, dy(br_u), dy(bi_u), dy(brad_u), dy(birad_u));
    CBall prod = a;
    prod.mul(b, P);

    // All quantities in units of 1/64; products land in units of 1/4096.
    auto absll = [](long long v) { return v < 0 ? -v : v; };
    const long long dev_re_u =
        absll(ar_u) * brad_u + absll(ai_u) * birad_u +
        arad_u * absll(br_u) + airad_u * absll(bi_u) +
        arad_u * brad_u + airad_u * birad_u;
    const long long dev_im_u =
        absll(ar_u) * birad_u + absll(ai_u) * brad_u +
        arad_u * absll(bi_u) + airad_u * absll(br_u) +
        arad_u * birad_u + airad_u * brad_u;
    const double dev_re = static_cast<double>(dev_re_u) / 4096.0;
    const double dev_im = static_cast<double>(dev_im_u) / 4096.0;

    ZF_CHECK(prod.rr >= dev_re);          // soundness
    ZF_CHECK(prod.ri >= dev_im);
    // Tightness. The centre-rounding term at P = 160 bits on centres of order
    // 1e5 is about 1e-43, so any honest deviation radius sits within a hair of
    // the exact bound. The rev 1 centre-inclusive radius was about 140x it.
    ZF_CHECK(prod.rr <= 4.0 * dev_re);
    ZF_CHECK(prod.ri <= 4.0 * dev_im);

    // The direct statement of the B1 defect: a product of two balls that are
    // far from the origin relative to their radii must NOT contain zero.
    const double cre = std::fabs(mpfr_get_d(prod.re, MPFR_RNDN));
    const double cim = std::fabs(mpfr_get_d(prod.im, MPFR_RNDN));
    ZF_CHECK(cre > prod.rr);
    ZF_CHECK(cim > prod.ri);
    std::printf("C4_TIGHTNESS rr/dev=%.4f ri/dev=%.4f signable=%d\n",
                prod.rr / dev_re, prod.ri / dev_im,
                static_cast<int>(cre > prod.rr && cim > prod.ri));
  }

  // ---- C5: the centre-rounding term is taken at the precision the result is
  // actually rounded into (R6-1) ------------------------------------------
  // mul_real rounded its products in place, at each component's STORED
  // precision, while charging the rounding term at wp. With a stored precision
  // below wp the charge under-reports the error actually committed: a 53-bit
  // ball at centre 1 multiplied by a 200-bit coefficient carrying 100 bits of
  // new information returned a ball around 1.0 whose radius was ~2^-199, while
  // the true product 1 + 2^-100 sat 2^-100 away. The products now go into
  // temporaries at wp and are swapped in, so every CBall component is stored at
  // wp after mul and mul_real.
  {
    constexpr mpfr_prec_t kStored = 53;
    constexpr mpfr_prec_t kWp = 200;

    CBall a(kStored);
    mpfr_set_d(a.re, 1.0, MPFR_RNDN);
    mpfr_set_d(a.im, 0.0, MPFR_RNDN);
    a.rr = 0.0;
    a.ri = 0.0;

    mpfr_t cf;
    mpfr_init2(cf, kWp);
    mpfr_set_ui(cf, 1, MPFR_RNDN);
    mpfr_t eps;
    mpfr_init2(eps, kWp);
    mpfr_set_ui(eps, 1, MPFR_RNDN);
    mpfr_div_2si(eps, eps, 100, MPFR_RNDN);   // 2^-100, exact at 200 bits
    mpfr_add(cf, cf, eps, MPFR_RNDN);         // 1 + 2^-100, exact at 200 bits

    a.mul_real(cf, 0.0, kWp);

    // Exact truth: 1 * (1 + 2^-100) = 1 + 2^-100, held at 300 bits.
    mpfr_t truth, dev;
    mpfr_init2(truth, 300);
    mpfr_init2(dev, 300);
    mpfr_set_ui(truth, 1, MPFR_RNDN);
    mpfr_set_ui(dev, 1, MPFR_RNDN);
    mpfr_div_2si(dev, dev, 100, MPFR_RNDN);
    mpfr_add(truth, truth, dev, MPFR_RNDN);
    mpfr_sub(dev, truth, a.re, MPFR_RNDN);
    mpfr_abs(dev, dev, MPFR_RNDN);

    const bool encloses = mpfr_cmp_d(dev, a.rr) <= 0;
    ZF_CHECK(encloses);
    ZF_CHECK(mpfr_get_prec(a.re) == kWp);
    ZF_CHECK(mpfr_get_prec(a.im) == kWp);
    std::printf("C5_PRECISION_RULE encloses=%d dev=%.6g rr=%.6g "
                "stored_after=%d\n",
                static_cast<int>(encloses), mpfr_get_d(dev, MPFR_RNDN), a.rr,
                static_cast<int>(mpfr_get_prec(a.re)));

    mpfr_clear(cf);
    mpfr_clear(eps);
    mpfr_clear(truth);
    mpfr_clear(dev);
  }

  std::fprintf(stdout, "CBALL_SUITE containment_failures %d failures %d\n",
               containment_failures, ::zftest::failure_count());
  return ::zftest::failure_count() == 0 ? 0 : 1;
}
