//
// Created by andy on 3/8/26.
//

#pragma once

#include <memory>
#include <neuron/api/api.hpp>

#include "config.hpp"
#include "render/render_context.hpp"

#include <list>
#include <unordered_set>

#if defined(WIN32)
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <Windows.h>
#endif

namespace neuron {
    class NEURON_API Module : public api::IModule {
      public:
        explicit Module(const std::filesystem::path &path);

        ~Module() override;
        const std::filesystem::path &path() const override;
        api::PFN_neuron_module_entry entry_point() const override;

      private:
        std::filesystem::path        _path;
        api::PFN_neuron_module_entry _entry;

#if defined(__linux__)
        void *_dl;
#elif defined(WIN32)
        HMODULE _module;
#else
#  error "Platform not supported"
#endif
    };

    class NEURON_API Engine : public std::enable_shared_from_this<Engine>, public api::IEngine {
        Engine();

      public:
        inline static std::shared_ptr<Engine> create() { return std::shared_ptr<Engine>(new Engine()); }

        ~Engine() override;

        Handle<api::IModule>                 load_module(const std::filesystem::path &path) override;
        bool                                 is_module_loaded(Handle<api::IModule> handle) override;
        bool                                 is_module_path_loaded(const std::filesystem::path &path) override;
        bool                                 unload_module(Handle<api::IModule> handle) override;
        Handle<api::IModule>                 find_loaded_module(const std::filesystem::path &path) override;
        const module_map_t                  &get_loaded_modules() override;
        std::shared_ptr<api::IRenderContext> render_context() override;

      private:
        std::unordered_set<api::IModule *> _loaded_modules;
        std::shared_ptr<RenderContext>     _render_context;
        module_map_t                       _modules_by_path;
    };
} // namespace neuron
