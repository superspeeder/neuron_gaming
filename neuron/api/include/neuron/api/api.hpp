//
// Created by andy on 3/7/26.
//

#pragma once
#include "handle.hpp"
#include "module.hpp"
#include "render.hpp"

#include <unordered_map>

namespace neuron::api {

    class IEngine : public std::enable_shared_from_this<IEngine> {
      public:
        virtual ~IEngine() = default;

        using module_map_t = std::unordered_map<std::filesystem::path, Handle<IModule>>;

        virtual Handle<IModule>     load_module(const std::filesystem::path &path)           = 0;
        virtual bool                is_module_loaded(Handle<IModule> handle)                 = 0;
        virtual bool                is_module_path_loaded(const std::filesystem::path &path) = 0;
        virtual bool                unload_module(Handle<IModule> handle)                    = 0;
        virtual Handle<IModule>     find_loaded_module(const std::filesystem::path &path)    = 0;
        virtual const module_map_t &get_loaded_modules()                                     = 0;

        virtual std::shared_ptr<IRenderContext> render_context() = 0;
    };
} // namespace neuron::api
