add_executable(bench bench.cpp)
target_link_libraries(bench libmim)
add_dependencies(bench mim_internal_core)
add_dependencies(mim_all bench)

add_executable(bench-regex regex.cpp)
target_link_libraries(bench-regex libmim)
add_dependencies(bench-regex mim_internal_core mim_internal_regex)
add_dependencies(mim_all bench-regex)
