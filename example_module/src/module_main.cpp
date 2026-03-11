//
// Created by andy on 3/8/26.
//

#include "neuron/api/api.hpp"


#include <iostream>
#include <neuron/api/module.hpp>

extern "C" void neuron_module_entry(neuron::api::IEngine *engine, neuron::api::ModuleLoadInfo load_info) {
    std::cout << "Shit" << std::endl;
    std::cout << load_info.filename << std::endl;
}
