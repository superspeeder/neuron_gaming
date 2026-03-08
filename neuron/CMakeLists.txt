cmake_minimum_required(VERSION 4.0)
project(neuron)

set(BUILD_SHARED_LIBS ON)

add_subdirectory(api)

find_package(VulkanHeaders REQUIRED)
find_package(glm CONFIG REQUIRED)

add_library(neuron
        src/neuron/neuron.cpp
        src/neuron/neuron.hpp
        src/neuron/config.hpp)
target_include_directories(neuron PUBLIC src)
target_link_libraries(neuron PUBLIC neuron::api)

add_library(neuron::neuron ALIAS neuron)

target_compile_features(neuron PUBLIC cxx_std_26)
