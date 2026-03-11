add_library(example_module SHARED src/module_main.cpp)
target_link_libraries(example_module PRIVATE neuron::api)
target_include_directories(example_module PRIVATE src)

if (LINUX)
    target_compile_options(example_module PRIVATE "-export-dynamic")
endif()