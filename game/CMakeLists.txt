project(game VERSION 1.0.0)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_executable(game src/game/main.cpp)
target_include_directories(game PRIVATE src)
target_link_libraries(game PRIVATE neuron::neuron)
target_compile_features(game PRIVATE cxx_std_23)

if (WIN32)
    add_custom_command(TARGET game POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:game> $<TARGET_RUNTIME_DLLS:game> COMMAND_EXPAND_LISTS)
endif()
