project(game VERSION 1.0.0)

add_executable(game src/game/main.cpp)
target_include_directories(game PRIVATE src)
target_link_libraries(game PRIVATE neuron::neuron)
target_compile_features(game PRIVATE cxx_std_26)
