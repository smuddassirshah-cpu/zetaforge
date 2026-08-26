#include "zetaforge/bernoulli.hpp"

#include <stdexcept>
#include <vector>

namespace zetaforge {

namespace {

// The ten values transcribed by hand in em_eval.cpp, repeated here as a
// tripwire on the recurrence. ATTACKS.md row 3 corrupts the em_eval copy; the
// cross-check below is what turns that into a failure.
constexpr long long kCheckNum[10] = {
    1, -1, 1, -1, 5, -691, 7, -3617, 43867, -174611};
constexpr long long kCheckDen[10] = {
    6, 30, 42, 30, 66, 2730, 6, 510, 798, 330};

// mpq_t is an array type, so it cannot be a vector element. The struct it
// wraps can: GMP values are trivially relocatable (the struct holds pointers to
// separately allocated limbs), so vector growth moving them is safe.
struct Table {
  std::vector<__mpq_struct> b;   // b[k] = B_k, k = 0 .. high
  unsigned high = 0;

  mpq_ptr at(size_t k) { return &b[k]; }

  Table() {
    b.resize(1);
    mpq_init(at(0));
    mpq_set_ui(at(0), 1, 1);   // B_0 = 1
  }

  ~Table() {
    for (size_t k = 0; k < b.size(); ++k) {
      mpq_clear(&b[k]);
    }
  }

  Table(const Table&) = delete;
  Table& operator=(const Table&) = delete;

  void ensure(unsigned m) {
    if (m <= high) {
      return;
    }
    mpz_t binom, num, den, tmp;
    mpz_inits(binom, num, den, tmp, nullptr);
    mpq_t term, acc;
    mpq_inits(term, acc, nullptr);

    for (unsigned k = high + 1; k <= m; ++k) {
      b.resize(k + 1);
      mpq_init(at(k));
      if (k % 2 == 1 && k > 1) {
        mpq_set_ui(at(k), 0, 1);       // B_odd = 0 for odd k > 1
        continue;
      }
      // B_k = -(1/(k+1)) * sum_{j<k} C(k+1, j) B_j
      mpq_set_ui(acc, 0, 1);
      for (unsigned j = 0; j < k; ++j) {
        if (mpq_sgn(at(j)) == 0) {
          continue;
        }
        mpz_bin_uiui(binom, k + 1, j);
        mpq_set_z(term, binom);
        mpq_mul(term, term, at(j));
        mpq_add(acc, acc, term);
      }
      mpq_set_ui(term, 1, k + 1);
      mpq_mul(acc, acc, term);
      mpq_neg(at(k), acc);
    }
    high = m;

    mpq_clears(term, acc, nullptr);
    mpz_clears(binom, num, den, tmp, nullptr);
  }
};

Table& table() {
  static Table t;
  return t;
}

// Runs once: the recurrence must reproduce the hand-transcribed table.
void verify_against_committed_table() {
  static bool done = false;
  if (done) {
    return;
  }
  Table& t = table();
  t.ensure(20);
  mpq_t expect;
  mpq_init(expect);
  for (unsigned n = 1; n <= 10; ++n) {
    mpq_set_si(expect, kCheckNum[n - 1],
               static_cast<unsigned long>(kCheckDen[n - 1]));
    mpq_canonicalize(expect);
    if (mpq_cmp(expect, t.at(2 * n)) != 0) {
      mpq_clear(expect);
      throw std::runtime_error(
          "Bernoulli recurrence disagrees with the committed table "
          "(MATHS.md D8, ATTACKS.md row 3)");
    }
  }
  mpq_clear(expect);
  done = true;
}

}  // namespace

void bernoulli_2n(unsigned n, mpq_t out) {
  if (n == 0 || n > kBernoulliMaxN) {
    throw std::out_of_range("bernoulli_2n index out of range");
  }
  verify_against_committed_table();
  Table& t = table();
  t.ensure(2 * n);
  mpq_set(out, t.at(2 * n));
}

}  // namespace zetaforge
