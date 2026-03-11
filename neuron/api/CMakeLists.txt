cmake_minimum_required(VERSION 4.0)
project(neuron_api VERSION 1.0.0)

set(BUILD_SHARED_LIBS ON)

find_package(glm CONFIG REQUIRED)
find_package(VulkanHeaders REQUIRED)

add_library(neuron_api INTERFACE
        include/neuron/api/module.hpp
        include/neuron/api/handle.hpp
        include/neuron/api/render.hpp)
add_library(neuron::api ALIAS neuron_api)
target_include_directories(neuron_api INTERFACE include/)
target_compile_features(neuron_api INTERFACE cxx_std_26)
target_link_libraries(neuron_api INTERFACE glm::glm Vulkan::Headers)
