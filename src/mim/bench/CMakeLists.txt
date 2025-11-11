add_executable(bench
    codegen.cpp
    main.cpp
)
target_link_libraries(bench libmim)
# set_target_properties(mim PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
# install(TARGETS mim EXPORT mim-targets)

add_dependencies(mim_all bench)
