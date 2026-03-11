//
// Created by andy on 3/8/26.
//

#include "neuron.hpp"

#include <GLFW/glfw3.h>
#include <iostream>

#if defined(__linux__)
#  include <dlfcn.h>
#elif defined(WIN32)
#  define NOMINMAX
#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#  include <libloaderapi.h>
#endif

namespace neuron {
    Module::Module(const std::filesystem::path &path) : _path(path) {
#if defined(__linux__)
        const auto patht = std::filesystem::absolute(path);
        _dl              = dlopen(patht.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (_dl == nullptr) {
            std::cerr << "Error loading module: " << dlerror() << std::endl;
        }
        void *entry_addr = dlsym(_dl, "neuron_module_entry");
        if (entry_addr == nullptr) {
            std::cerr << "Error loading module entry point: " << dlerror() << std::endl;
        }
        _entry = reinterpret_cast<api::PFN_neuron_module_entry>(entry_addr);
#elif defined(WIN32)
        const auto patht = std::filesystem::absolute(path);
        _module          = LoadLibraryW(patht.c_str());
        if (_module == nullptr) {
            DWORD err = GetLastError();
            LPSTR msg = nullptr;
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, err, 0, msg, 0,
                           nullptr);
            std::cerr << "Error loading module: " << msg << std::endl;
            LocalFree(msg);
        }

        const FARPROC entry_addr = GetProcAddress(_module, "neuron_module_entry");
        if (entry_addr == nullptr) {
            std::cerr << "Error loading module entry point" << std::endl;
        }
        std::cout << "Proc: " << reinterpret_cast<uintptr_t>(entry_addr) << std::endl;
        _entry = reinterpret_cast<api::PFN_neuron_module_entry>(entry_addr);
#else
        throw std::logic_error("Unimplemented");
#endif
    }

    Module::~Module() {
#if defined(__linux__)
        dlclose(_dl);
#elif defined(WIN32)
        FreeLibrary(_module);
#endif
    }

    const std::filesystem::path &Module::path() const {
        return _path;
    }

    api::PFN_neuron_module_entry Module::entry_point() const {
        return _entry;
    }

    Engine::Engine() {
        glfwInit();

        _render_context = std::make_shared<RenderContext>();
    }

    Engine::~Engine() {
        for (const auto *module : _loaded_modules) {
            delete module;
        }
        _loaded_modules.clear();
    }

    Handle<api::IModule> Engine::load_module(const std::filesystem::path &path) {
        if (const auto handle = find_loaded_module(path)) {
            return handle;
        }

        auto module = new Module(path);
        _loaded_modules.emplace(module);
        auto module_handle = Handle(module);
        _modules_by_path.insert({path, module_handle});
        return module_handle;
    }

    bool Engine::is_module_loaded(Handle<api::IModule> handle) {
        return _loaded_modules.contains(handle.get());
    }

    bool Engine::is_module_path_loaded(const std::filesystem::path &path) {
        return _modules_by_path.contains(path);
    }

    bool Engine::unload_module(Handle<api::IModule> handle) {
        if (const auto it = _loaded_modules.find(handle.get()); it != _loaded_modules.end()) {
            const auto path = (*it)->path();
            delete *it;
            _loaded_modules.erase(it);
            _modules_by_path.erase(path);
            return true;
        }

        return false;
    }

    Handle<api::IModule> Engine::find_loaded_module(const std::filesystem::path &path) {
        if (const auto it = _modules_by_path.find(path); it != _modules_by_path.end()) {
            return it->second;
        }
        return Handle<api::IModule>(nullptr);
    }

    const api::IEngine::module_map_t &Engine::get_loaded_modules() {
        return _modules_by_path;
    }

    std::shared_ptr<api::IRenderContext> Engine::render_context() {
        return _render_context;
    }
} // namespace neuron
