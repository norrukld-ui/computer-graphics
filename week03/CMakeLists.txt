add_executable(week03 main.cpp shader_program.cpp)
target_link_libraries(week03 PRIVATE cg_deps)

# Шейдеры читаются прямо из исходной папки: можно поправить .frag,
# нажать R в работающей программе и сразу увидеть результат без пересборки.
target_compile_definitions(week03 PRIVATE
    WEEK03_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders/")
