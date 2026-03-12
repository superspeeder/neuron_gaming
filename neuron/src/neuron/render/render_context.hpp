//
// Created by andy on 3/10/26.
//

#pragma once
#include "neuron/config.hpp"
#include <neuron/api/render.hpp>
#include <set>

#include <GLFW/glfw3.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winrt/Windows.Foundation.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
// See https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/ui/apply-windows-themes
#include <windows.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <dwmapi.h>
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

namespace neuron {

    class NEURON_API RenderContext;

    class NEURON_API Window : public api::IWindow, public api::ISwapchain {
      public:
        Window(std::string_view title, uint32_t width, uint32_t height, const std::shared_ptr<RenderContext> &context);
        ~Window() override;

        [[nodiscard]] vk::Extent2D window_extent() override;
        [[nodiscard]] bool         should_close() override;
        void                       close() override;
        void                       update_events() override;

        const vk::raii::SurfaceKHR   &surface() override;
        const vk::raii::SwapchainKHR &swapchain() override;
        const std::vector<vk::Image> &images() override;
        const vk::Extent2D           &extent() override;
        const vk::SurfaceFormatKHR   &surface_format() override;
        vk::PresentModeKHR            present_mode() override;
        void                          set_format_selector(const surface_format_selector_t &format_selector) override;
        void                          set_present_mode_selector(const present_mode_selector_t &present_mode_selector) override;
        void                          set_image_usage(vk::ImageUsageFlags usage) override;
        void                          set_clipped(bool clipped) override;
        void                          set_composite_alpha(vk::CompositeAlphaFlagBitsKHR composite_alpha) override;
        void                          reconfigure() override;
        api::SwapchainFrameInfo       acquire_next_frame(vk::Semaphore image_available) override;
        void                          present_image(const api::SwapchainFrameInfo &frame_info, vk::Semaphore can_present) override;

      private:
        GLFWwindow *_window;

        vk::raii::SurfaceKHR   _surface{nullptr};
        vk::raii::SwapchainKHR _swapchain{nullptr};
        std::vector<vk::Image> _images;

        surface_format_selector_t _surface_format_selector = default_format_selector;
        present_mode_selector_t   _present_mode_selector   = default_present_mode_selector;

        vk::SurfaceFormatKHR           _surface_format;
        vk::PresentModeKHR             _present_mode;
        vk::Extent2D                   _extent;
        vk::ImageUsageFlags            _image_usage     = vk::ImageUsageFlagBits::eColorAttachment;
        bool                           _clipped         = false;
        vk::CompositeAlphaFlagBitsKHR  _composite_alpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        std::shared_ptr<RenderContext> _render_context;

        static vk::SurfaceFormatKHR default_format_selector(const std::vector<vk::SurfaceFormatKHR> &formats);
        static vk::PresentModeKHR   default_present_mode_selector(const std::vector<vk::PresentModeKHR> &modes);

#ifdef WIN32
        winrt::Windows::UI::ViewManagement::UISettings::ColorValuesChanged_revoker _dark_mode_revoker;
#endif
    };

    class NEURON_API RenderContext : public api::IRenderContext {
      public:
        RenderContext();

        ~RenderContext() override;
        const vk::raii::Instance       &instance() override;
        const vk::raii::PhysicalDevice &physical_device() override;
        const vk::raii::Device         &device() override;
        const vk::raii::Queue          &graphics_queue() override;
        const vk::raii::Queue          &present_queue() override;
        const vk::raii::Queue          &transfer_queue() override;
        const vk::raii::Queue          &compute_queue() override;
        uint32_t                        graphics_family() override;
        uint32_t                        present_family() override;
        uint32_t                        transfer_family() override;
        uint32_t                        compute_family() override;

        std::vector<const char *> enabled_extensions() override;
        bool                      is_extension_enabled(std::string_view extension) override;

        std::tuple<std::shared_ptr<api::IWindow>, std::shared_ptr<api::ISwapchain>> create_window(std::string_view title, const vk::Extent2D &size) override;

      private:
        vk::raii::Context         _context;
        vk::raii::Instance        _instance{nullptr};
        vk::raii::PhysicalDevice  _physical_device{nullptr};
        vk::raii::Device          _device{nullptr};
        vk::raii::Queue           _graphics_queue{nullptr};
        vk::raii::Queue           _present_queue{nullptr};
        vk::raii::Queue           _transfer_queue{nullptr};
        vk::raii::Queue           _compute_queue{nullptr};
        uint32_t                  _graphics_family = 0;
        uint32_t                  _present_family  = 0;
        uint32_t                  _transfer_family = 0;
        uint32_t                  _compute_family  = 0;
        std::vector<const char *> _enabled_extensions;
        std::set<std::string>     _enabled_extension_set;
    };

    class NEURON_API DescriptorPool {

    };
} // namespace neuron
