//
// Created by andy on 3/10/26.
//

#pragma once
#include "handle.hpp"


#include <memory>

#include <functional>
#include <vulkan/vulkan_raii.hpp>

namespace neuron::api {
    class IWindow;
    class ISwapchain;
    class ICommandPool;
    class ICommandBuffer;

    class IRenderContext : public std::enable_shared_from_this<IRenderContext> {
      public:
        virtual ~IRenderContext() = default;

        virtual const vk::raii::Instance       &instance()        = 0;
        virtual const vk::raii::PhysicalDevice &physical_device() = 0;
        virtual const vk::raii::Device         &device()          = 0;
        virtual const vk::raii::Queue          &graphics_queue()  = 0;
        virtual const vk::raii::Queue          &present_queue()   = 0;
        virtual const vk::raii::Queue          &transfer_queue()  = 0;
        virtual const vk::raii::Queue          &compute_queue()   = 0;
        virtual uint32_t                        graphics_family() = 0;
        virtual uint32_t                        present_family()  = 0;
        virtual uint32_t                        transfer_family() = 0;
        virtual uint32_t                        compute_family()  = 0;

        virtual std::vector<const char *> enabled_extensions() = 0;
        virtual bool is_extension_enabled(std::string_view extension) = 0;

        virtual std::tuple<std::shared_ptr<IWindow>, std::shared_ptr<ISwapchain>> create_window(std::string_view title, const vk::Extent2D &size) = 0;

      private:
    };

    class IWindow : public std::enable_shared_from_this<IWindow> {
      public:
        virtual ~IWindow() = default;

        [[nodiscard]] virtual vk::Extent2D window_extent() = 0;
        [[nodiscard]] virtual bool         should_close()  = 0;

        virtual void close()         = 0;
        virtual void update_events() = 0;

      private:
    };

    struct SwapchainFrameInfo {
        vk::Image         image;
        uint32_t          image_index;
        vk::Extent2D      extent;
        vk::Format        format;
        vk::ColorSpaceKHR color_space;
    };

    class ISwapchain : public std::enable_shared_from_this<ISwapchain> {
      public:
        virtual ~ISwapchain() = default;

        virtual const vk::raii::SurfaceKHR   &surface()   = 0;
        virtual const vk::raii::SwapchainKHR &swapchain() = 0;

        virtual const std::vector<vk::Image> &images()         = 0;
        virtual const vk::Extent2D           &extent()         = 0;
        virtual const vk::SurfaceFormatKHR   &surface_format() = 0;
        virtual vk::PresentModeKHR            present_mode()   = 0;

        using surface_format_selector_t = std::function<vk::SurfaceFormatKHR(const std::vector<vk::SurfaceFormatKHR> &)>;
        using present_mode_selector_t   = std::function<vk::PresentModeKHR(const std::vector<vk::PresentModeKHR> &)>;

        virtual void set_format_selector(const surface_format_selector_t &format_selector)           = 0;
        virtual void set_present_mode_selector(const present_mode_selector_t &present_mode_selector) = 0;
        virtual void set_image_usage(vk::ImageUsageFlags usage)                                      = 0;
        virtual void set_clipped(bool clipped)                                                       = 0;
        virtual void set_composite_alpha(vk::CompositeAlphaFlagBitsKHR composite_alpha)              = 0;

        virtual void reconfigure() = 0;

        virtual SwapchainFrameInfo acquire_next_frame(vk::Semaphore image_available)                              = 0;
        virtual void               present_image(const SwapchainFrameInfo &frame_info, vk::Semaphore can_present) = 0;

      private:
    };

    class ICommandPool {
    public:
        virtual ~ICommandPool() = default;

        virtual Handle<ICommandBuffer> allocate_buffer() = 0;
        virtual std::vector<Handle<ICommandBuffer>> allocate_buffers(uint32_t count) = 0;
    };

    class ICommandBuffer {
    public:
        virtual ~ICommandBuffer() = default;
    };

    class IDescriptorPool {
    public:
        virtual ~IDescriptorPool() = default;



    private:
        vk::raii::DescriptorPool descriptor_pool;
    };
} // namespace neuron::api
