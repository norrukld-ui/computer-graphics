# Вспомогательный код (окно + шейдеры) общий для обеих программ недели.
add_library(week02_helpers STATIC gl_helpers.cpp)
target_link_libraries(week02_helpers PUBLIC cg_deps)

add_executable(week02_triangles triangles.cpp)
target_link_libraries(week02_triangles PRIVATE week02_helpers)

add_executable(week02_grid grid.cpp)
target_link_libraries(week02_grid PRIVATE week02_helpers)
