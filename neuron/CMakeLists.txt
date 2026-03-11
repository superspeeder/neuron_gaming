cmake_minimum_required(VERSION 4.0)
project(neuron)

set(BUILD_SHARED_LIBS ON)

add_subdirectory(api)

find_package(VulkanHeaders REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_library(neuron
        src/neuron/neuron.cpp
        src/neuron/neuron.hpp
        src/neuron/config.hpp
        src/neuron/render/render_context.cpp
        src/neuron/render/render_context.hpp)
target_include_directories(neuron PUBLIC src)
target_link_libraries(neuron PUBLIC neuron::api glm::glm Vulkan::Headers glfw)

add_library(neuron::neuron ALIAS neuron)

if (LINUX)
    target_link_libraries(neuron PUBLIC dl)
endif()

if (WIN32)
    target_compile_definitions(neuron PUBLIC NEURON_SHARED PRIVATE NEURON_EXPORTS)
    target_link_libraries(neuron PUBLIC dwmapi RuntimeObject)


    add_custom_command(TARGET neuron::neuron POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:game> $<TARGET_FILE:neuron::neuron> COMMAND_EXPAND_LISTS)
endif()

target_compile_features(neuron PUBLIC cxx_std_23)
