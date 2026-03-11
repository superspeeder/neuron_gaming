//
// Created by andy on 3/8/26.
//

#pragma once

#include <filesystem>

namespace neuron::api {
    class IEngine;

    struct ModuleLoadInfo {
        std::filesystem::path filename;
    };

    using PFN_neuron_module_entry = void (*)(IEngine *engine, ModuleLoadInfo load_info);

    class IModule {
    public:
        virtual ~IModule() = default;

        virtual const std::filesystem::path& path() const = 0;
        virtual PFN_neuron_module_entry entry_point() const = 0;
    };
} // namespace neuron::api
