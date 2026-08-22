#pragma once

// Decision note: stage 1 pipeline proof only. The double-based placeholder
// exercises the build, test, and CI path; the mpfr-backed ball with directed
// rounding and precision-ladder escalation lands in stage 2 per
// docs/PLAN.md section 11.

namespace zetaforge {

class Ball {
public:
  Ball(double centre, double radius);
  [[nodiscard]] double centre() const noexcept;
  [[nodiscard]] double radius() const noexcept;
  [[nodiscard]] bool contains_zero() const noexcept;
  [[nodiscard]] double width() const noexcept;

private:
  double centre_;
  double radius_;
};

}  // namespace zetaforge
