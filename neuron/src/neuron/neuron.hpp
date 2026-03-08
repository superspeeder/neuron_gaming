//
// Created by andy on 3/8/26.
//

#pragma once

#include <memory>
#include <neuron/api/api.hpp>

#include "config.hpp"

namespace neuron {
    class Engine : public std::enable_shared_from_this<Engine>, api::IEngine {
        Engine();
    public:
        inline static std::shared_ptr<Engine> create() {
            return std::shared_ptr<Engine>(new Engine());
        }

        ~Engine() override;

    private:

    };
}
