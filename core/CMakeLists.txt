option(ZF_SABOTAGE_HOOKS "Compile-time sabotage hooks (NEVER enable in production)" OFF)

add_library(zetaforge_core STATIC src/ball.cpp src/bernoulli.cpp src/em_eval.cpp src/theta.cpp)
target_include_directories(zetaforge_core PUBLIC include)
target_compile_features(zetaforge_core PUBLIC cxx_std_23)
target_link_libraries(zetaforge_core PUBLIC PkgConfig::MPFR PkgConfig::GMP)

# Sabotage wiring (review finding C2). The define goes on the LIBRARY, which is
# where the hooked code lives, and PUBLIC so consuming tests agree with it. The
# previous wiring put the define on a test target that had not been declared
# yet: it failed to configure, and had it configured, the knob still could not
# have reached theta.cpp inside the library.
if(ZF_SABOTAGE_HOOKS)
  target_sources(zetaforge_core PRIVATE src/sabotage.cpp)
  target_compile_definitions(zetaforge_core PUBLIC ZF_SABOTAGE_HOOKS)
  message(WARNING "ZF_SABOTAGE_HOOKS is ON: this build is for gate-battery use only, never production")
endif()

add_executable(test_ball tests/test_ball.cpp)
target_link_libraries(test_ball PRIVATE zetaforge_core)
add_test(NAME ball COMMAND test_ball)

add_executable(test_ball_oracle tests/test_ball_oracle.cpp)
target_include_directories(test_ball_oracle PRIVATE ${FLINT_INCLUDE_DIR})
target_link_libraries(test_ball_oracle PRIVATE zetaforge_core ${FLINT_LIBRARY})
add_test(NAME ball_oracle COMMAND test_ball_oracle)

add_executable(test_ffix tests/test_ffix.cpp)
target_link_libraries(test_ffix PRIVATE zetaforge_core)
add_test(NAME ffix COMMAND test_ffix)

add_executable(test_ntt tests/test_ntt.cpp)
target_link_libraries(test_ntt PRIVATE zetaforge_core)
add_test(NAME ntt COMMAND test_ntt)

# Determinism probe: not a pass/fail test on its own; CI extracts
# DETERMINISM_HASH from both build configurations and requires equality.
add_executable(test_determinism tests/test_determinism.cpp)
target_include_directories(test_determinism PRIVATE ${FLINT_INCLUDE_DIR})
target_link_libraries(test_determinism PRIVATE zetaforge_core ${FLINT_LIBRARY})
add_test(NAME determinism_selfconsistency
         COMMAND test_determinism)

add_executable(bench_ntt bench/bench_ntt.cpp)
target_link_libraries(bench_ntt PRIVATE zetaforge_core)

add_executable(test_radius tests/test_radius.cpp)
target_link_libraries(test_radius PRIVATE zetaforge_core)
add_test(NAME radius_exact COMMAND test_radius)

add_executable(test_radius_invariants tests/test_radius_invariants.cpp)
target_link_libraries(test_radius_invariants PRIVATE zetaforge_core)
add_test(NAME radius_invariants COMMAND test_radius_invariants)

# Stage 4 suite (review finding C1). Built always so it cannot rot, but
# registered with ctest only when explicitly armed: it is pre-registered
# against a definition of done that no code satisfies yet, so with the current
# throwing stub an armed run exits 1. A green default suite must never be
# readable as "stage 4 works".
option(ZF_ARM_STAGE4 "Register the stage 4 Z(t) suite with ctest (RED until the EM path lands)" OFF)
add_executable(test_zeta tests/test_zeta.cpp)
target_link_libraries(test_zeta PRIVATE zetaforge_core ${FLINT_LIBRARY})
target_include_directories(test_zeta PRIVATE ${FLINT_INCLUDE_DIR})
if(ZF_ARM_STAGE4)
  add_test(NAME zeta COMMAND test_zeta)
endif()

add_executable(test_theta tests/test_theta.cpp)
target_link_libraries(test_theta PRIVATE zetaforge_core ${FLINT_LIBRARY})
target_include_directories(test_theta PRIVATE ${FLINT_INCLUDE_DIR})
target_compile_definitions(test_theta PRIVATE
  THETA_GOLDEN_CSV="${CMAKE_SOURCE_DIR}/docs/golden/theta_golden.csv"
  THETA_GOLDEN_SUBT0_CSV="${CMAKE_SOURCE_DIR}/docs/golden/theta_golden_subt0.csv")

add_test(NAME theta COMMAND test_theta)

add_executable(test_cball tests/test_cball.cpp)
target_link_libraries(test_cball PRIVATE zetaforge_core)
add_test(NAME cball COMMAND test_cball)
