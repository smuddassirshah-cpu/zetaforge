add_library(zetaforge_core STATIC src/ball.cpp src/em_eval.cpp src/theta.cpp)
target_include_directories(zetaforge_core PUBLIC include)
target_compile_features(zetaforge_core PUBLIC cxx_std_23)
option(ZF_SABOTAGE_HOOKS "Compile-time sabotage hooks (NEVER enable in production)" OFF)
target_link_libraries(zetaforge_core PUBLIC PkgConfig::MPFR PkgConfig::GMP)

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

add_executable(test_zeta tests/test_zeta.cpp)
target_link_libraries(test_zeta PRIVATE zetaforge_core ${FLINT_LIBRARY})
target_include_directories(test_zeta PRIVATE ${FLINT_INCLUDE_DIR})
add_test(NAME zeta COMMAND test_zeta)

if(ZF_SABOTAGE_HOOKS)
  target_compile_definitions(test_theta PRIVATE ZF_SABOTAGE_HOOKS)
endif()
add_executable(test_theta tests/test_theta.cpp)
target_link_libraries(test_theta PRIVATE zetaforge_core)
target_compile_definitions(test_theta PRIVATE
  THETA_GOLDEN_CSV="${CMAKE_SOURCE_DIR}/docs/golden/theta_golden.csv")

add_test(NAME theta COMMAND test_theta)

add_executable(test_cball tests/test_cball.cpp)
target_link_libraries(test_cball PRIVATE zetaforge_core)
add_test(NAME cball COMMAND test_cball)
