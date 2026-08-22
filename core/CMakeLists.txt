add_library(zetaforge_core STATIC src/ball.cpp)
target_include_directories(zetaforge_core PUBLIC include)
target_compile_features(zetaforge_core PUBLIC cxx_std_23)

add_executable(test_ball tests/test_ball.cpp)
target_link_libraries(test_ball PRIVATE zetaforge_core)
add_test(NAME ball COMMAND test_ball)
