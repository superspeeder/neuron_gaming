//
// Created by andy on 3/10/26.
//

#include "render_context.hpp"

#include <array>
#include <iostream>
#include <ranges>
#include <set>

#ifdef WIN32
using namespace winrt;
using namespace Windows::UI::ViewManagement;

static bool is_color_light(const Windows::UI::Color &clr) {
    return (((5 * clr.G) + (2 * clr.R) + clr.B) > (8 * 128));
}

static bool is_dark_mode() {
    const auto settings = UISettings();

    const auto foreground = settings.GetColorValue(UIColorType::Foreground);
    return static_cast<bool>(is_color_light(foreground));
}

static auto set_dark_mode_revoker(std::function<void(bool)> f) {
    auto settings = UISettings();
    auto revoker  = settings.ColorValuesChanged(winrt::auto_revoke, [settings, f](auto &&...) {
        const auto foreground = settings.GetColorValue(UIColorType::Foreground);
        const bool is_dark    = is_color_light(foreground);
        f(is_dark);
    });

    return revoker;
}

static void set_dark_mode_for_window(GLFWwindow *window, bool set) {
    const BOOL                  val = set;
    [[maybe_unused]] const auto _   = DwmSetWindowAttribute(glfwGetWin32Window(window), DWMWA_USE_IMMERSIVE_DARK_MODE, &val, sizeof(val));
}
#endif

namespace neuron {
    Window::Window(const std::string_view title, const uint32_t width, const uint32_t height, const std::shared_ptr<RenderContext> &context) : _render_context(context) {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        _window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.data(), nullptr, nullptr);

#ifdef WIN32
        set_dark_mode_for_window(_window, is_dark_mode());
        _dark_mode_revoker = set_dark_mode_revoker([this](const bool set) { set_dark_mode_for_window(_window, set); });
#endif

        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(*_render_context->instance(), _window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vulkan surface for window");
        }

        _surface = vk::raii::SurfaceKHR(_render_context->instance(), surface);

        Window::reconfigure();
    }

    Window::~Window() {
        glfwDestroyWindow(_window);
    }

    vk::Extent2D Window::window_extent() {
        int w, h;
        glfwGetFramebufferSize(_window, &w, &h);
        return {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    }

    bool Window::should_close() {
        return glfwWindowShouldClose(_window);
    }

    void Window::close() {
        glfwSetWindowShouldClose(_window, true);
    }

    void Window::update_events() {
        glfwPollEvents();
    }

    const vk::raii::SurfaceKHR &Window::surface() {
        return _surface;
    }

    const vk::raii::SwapchainKHR &Window::swapchain() {
        return _swapchain;
    }

    const std::vector<vk::Image> &Window::images() {
        return _images;
    }

    const vk::Extent2D &Window::extent() {
        return _extent;
    }

    const vk::SurfaceFormatKHR &Window::surface_format() {
        return _surface_format;
    }

    vk::PresentModeKHR Window::present_mode() {
        return _present_mode;
    }

    void Window::set_format_selector(const surface_format_selector_t &format_selector) {
        _surface_format_selector = format_selector;
    }

    void Window::set_present_mode_selector(const present_mode_selector_t &present_mode_selector) {
        _present_mode_selector = present_mode_selector;
    }

    void Window::set_image_usage(const vk::ImageUsageFlags usage) {
        _image_usage = usage;
    }

    void Window::set_clipped(bool clipped) {
        _clipped = clipped;
    }

    void Window::set_composite_alpha(vk::CompositeAlphaFlagBitsKHR composite_alpha) {
        _composite_alpha = composite_alpha;
    }

    void Window::reconfigure() {
        vk::SwapchainCreateInfoKHR create_info{};
        create_info.clipped          = _clipped;
        create_info.compositeAlpha   = _composite_alpha;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage       = _image_usage;
        create_info.surface          = *_surface;

        std::vector<uint32_t> families;
        if (_render_context->graphics_family() != _render_context->present_family()) {
            create_info.imageSharingMode = vk::SharingMode::eConcurrent;
            families                     = {_render_context->graphics_family(), _render_context->present_family()};
            create_info.setQueueFamilyIndices(families);
        } else {
            families                     = {};
            create_info.imageSharingMode = vk::SharingMode::eExclusive;
        }

        const auto caps = _render_context->physical_device().getSurfaceCapabilitiesKHR(_surface);

        if (caps.currentExtent.width == UINT32_MAX) {
            auto wext      = window_extent();
            _extent.width  = std::clamp<uint32_t>(wext.width, caps.minImageExtent.width, caps.maxImageExtent.width);
            _extent.height = std::clamp<uint32_t>(wext.height, caps.minImageExtent.height, caps.maxImageExtent.height);
        } else {
            _extent = caps.currentExtent;
        }

        create_info.imageExtent = _extent;

        const auto modes        = _render_context->physical_device().getSurfacePresentModesKHR(_surface);
        _present_mode           = _present_mode_selector(modes);
        create_info.presentMode = _present_mode;

        const auto formats          = _render_context->physical_device().getSurfaceFormatsKHR(_surface);
        _surface_format             = _surface_format_selector(formats);
        create_info.imageFormat     = _surface_format.format;
        create_info.imageColorSpace = _surface_format.colorSpace;

        create_info.minImageCount = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && create_info.minImageCount > caps.maxImageCount) {
            create_info.minImageCount = caps.maxImageCount;
        }

        create_info.preTransform = caps.currentTransform;

        if (*_swapchain) {
            create_info.oldSwapchain = *_swapchain;
        }

        vk::raii::SwapchainKHR swapchain(_render_context->device(), create_info);
        _swapchain.swap(swapchain);

        _images = _swapchain.getImages();
    }

    api::SwapchainFrameInfo Window::acquire_next_frame(const vk::Semaphore image_available) {
        auto [res, index] = _swapchain.acquireNextImage(UINT64_MAX, image_available, nullptr);
        return api::SwapchainFrameInfo{
            _images[index], index, _extent, _surface_format.format, _surface_format.colorSpace,
        };
    }

    void Window::present_image(const api::SwapchainFrameInfo &frame_info, const vk::Semaphore can_present) {
        vk::PresentInfoKHR present_info{};
        present_info.setImageIndices(frame_info.image_index);
        present_info.setSwapchains(*_swapchain);
        present_info.setWaitSemaphores(can_present);
        [[maybe_unused]] auto res = _render_context->present_queue().presentKHR(present_info);
    }

    vk::SurfaceFormatKHR Window::default_format_selector(const std::vector<vk::SurfaceFormatKHR> &formats) {
        for (const auto &[format, colorSpace] : formats) {
            if (colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear && format == vk::Format::eB8G8R8A8Srgb) {
                return {format, colorSpace};
            }
        }

        return formats[0];
    }

    vk::PresentModeKHR Window::default_present_mode_selector(const std::vector<vk::PresentModeKHR> &modes) {
        for (const auto &mode : modes) {
            if (mode == vk::PresentModeKHR::eMailbox) {
                return mode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    RenderContext::RenderContext() {
        {
            vk::ApplicationInfo app_info{};
            app_info.apiVersion = vk::ApiVersion14;
            vk::InstanceCreateInfo create_info{};
            create_info.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&create_info.enabledExtensionCount);
            create_info.pApplicationInfo        = &app_info;
            _instance                           = vk::raii::Instance(_context, create_info);
        }

        {
            _physical_device = _instance.enumeratePhysicalDevices()[0];
        }

        {
            std::optional<uint32_t> graphics_family;
            std::optional<uint32_t> present_family;
            std::optional<uint32_t> transfer_family;
            std::optional<uint32_t> compute_family;

            for (const auto queue_families = _physical_device.getQueueFamilyProperties(); const auto &[index, props] : queue_families | std::views::enumerate) {
                if (props.queueFlags & vk::QueueFlagBits::eGraphics && !graphics_family.has_value()) {
                    graphics_family = index;
                    if (glfwGetPhysicalDevicePresentationSupport(*_instance, *_physical_device, index)) {
                        present_family = index;
                    }
                }
                if (!present_family.has_value() && glfwGetPhysicalDevicePresentationSupport(*_instance, *_physical_device, index)) {
                    present_family = index;
                }
                if (!transfer_family.has_value() && !(props.queueFlags & vk::QueueFlagBits::eGraphics) && (props.queueFlags & vk::QueueFlagBits::eTransfer)) {
                    transfer_family = index;
                }
                if (props.queueFlags & vk::QueueFlagBits::eCompute && !compute_family.has_value()) {
                    compute_family = index;
                }
            }

            if (!graphics_family.has_value()) {
                throw std::runtime_error("Graphics is not supported");
            }

            if (!compute_family.has_value()) {
                throw std::runtime_error("GPGPU is not supported (potentially broken Vulkan driver)");
            }

            if (!present_family.has_value()) {
                throw std::runtime_error("Surface presentation is not supported (potentially broken window system).");
            }

            _graphics_family = graphics_family.value();
            _transfer_family = transfer_family.value_or(_graphics_family);
            _present_family  = present_family.value();
            _compute_family  = compute_family.value();
        }

        {
            const std::set                         queue_indices = {_graphics_family, _present_family, _transfer_family, _compute_family};
            std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
            std::array                             queue_priorities = {1.0f};
            for (const uint32_t index : queue_indices) {
                queue_create_infos.emplace_back(vk::DeviceQueueCreateFlags{}, index, queue_priorities);
            }

            vk::DeviceCreateInfo      create_info{};
            std::vector<const char *> extensions = {};

            std::set<std::string> desired_extensions = {
                "VK_KHR_buffer_device_address",
                "VK_KHR_copy_commands2",
                "VK_KHR_external_fence_fd",
                "VK_KHR_maintenance1",
                "VK_KHR_maintenance2",
                "VK_KHR_maintenance3",
                "VK_KHR_maintenance4",
                "VK_KHR_maintenance5",
                "VK_KHR_maintenance6",
                "VK_KHR_maintenance8",
                "VK_KHR_maintenance9",
                "VK_KHR_maintenance10",

                "VK_KHR_dynamic_rendering",
                "VK_KHR_push_descriptor",
                "VK_KHR_shader_integer_dot_product",
                "VK_KHR_variable_pointers",
                "VK_KHR_create_renderpass2",
                "VK_KHR_spirv_1_4",
                "VK_KHR_multiview",
                "VK_KHR_shader_draw_parameters",

                "VK_KHR_synchronization2",
                "VK_KHR_device_group",
                "VK_KHR_maintenance4",
                "VK_KHR_load_store_op_none",
                "VK_KHR_timeline_semaphore",
                "VK_EXT_inline_uniform_block",
                "VK_EXT_load_store_op_none",

                "VK_KHR_swapchain",
                "VK_KHR_display_swapchain",
                "VK_EXT_conditional_rendering",
                "VK_EXT_display_control",
                "VK_EXT_discard_rectangles",
                "VK_EXT_conservative_rasterization",
                "VK_KHR_performance_query",
                "VK_EXT_descriptor_heap",
                "VK_EXT_blend_operation_advanced",
                "VK_NV_fill_rectangle",
                "VK_AMD_device_coherent_memory",
                "VK_EXT_memory_budget",
                "VK_EXT_memory_priority",
                "VK_EXT_full_screen_exclusive",
                "VK_KHR_deferred_host_operations",
                "VK_NV_device_generated_commands",
                "VK_EXT_device_memory_report",
                "VK_KHR_maintenance7",
                "VK_EXT_transform_feedback",
                "VK_KHR_incremental_present",
                "VK_EXT_depth_clip_enable",
                "VK_EXT_hdr_metadata",
                "VK_EXT_shader_stencil_export",
                "VK_EXT_sample_locations",
                "VK_EXT_filter_cubic",
                "VK_EXT_external_memory_host",
                "VK_KHR_shader_clock",
                "VK_KHR_swapchain_mutable_format",
                "VK_NV_mesh_shader",
                "VK_EXT_present_timing",
                "VK_EXT_pci_bus_info",
                "VK_EXT_fragment_density_map",
                "VK_KHR_shader_quad_control",
                "VK_KHR_present_wait",
                "VK_EXT_provoking_vertex",
                "VK_KHR_pipeline_executable_properties",
                "VK_EXT_depth_bias_control",
                "VK_KHR_pipeline_library",
                "VK_KHR_present_id",
                "VK_NV_device_diagnostics_config",
                "VK_EXT_graphics_pipeline_library",
                "VK_KHR_shader_subgroup_uniform_control_flow",
                "VK_EXT_mesh_shader",
                "VK_EXT_image_compression_control",
                "VK_EXT_device_fault",
                "VK_EXT_vertex_input_dynamic_state",
                "VK_EXT_device_address_binding_report",
                "VK_EXT_depth_clip_control",
                "VK_EXT_primitive_topology_list_restart",
                "VK_EXT_pipeline_properties",
                "VK_EXT_multisampled_render_to_single_sampled",
                "VK_EXT_color_write_enable",
                "VK_EXT_primitives_generated_query",
                "VK_EXT_multi_draw",
                "VK_EXT_image_2d_view_of_3d",
                "VK_NV_device_generated_commands_compute",
                "VK_EXT_nested_command_buffer",
                "VK_EXT_extended_dynamic_state3",
                "VK_EXT_shader_module_identifier",
                "VK_KHR_present_id2",
                "VK_KHR_present_wait2",
                "VK_EXT_shader_object",
                "VK_KHR_pipeline_binary",
                "VK_KHR_swapchain_maintenance1",
                "VK_EXT_mutable_descriptor_type",
                "VK_KHR_compute_shader_derivatives",
                "VK_KHR_unified_image_layouts",
                "VK_EXT_memory_decompression",
                "VK_KHR_copy_memory_indirect",
                "VK_EXT_device_generated_commands",
                "VK_NV_push_constant_bank",
                "VK_KHR_format_feature_flags2",
                "VK_KHR_driver_properties",
                "VK_EXT_descriptor_buffer",
                "VK_EXT_extended_dynamic_state",
                "VK_EXT_descriptor_indexing",
                "VK_KHR_vulkan_memory_model",
                "VK_KHR_imageless_framebuffer",
                "VK_KHR_dedicated_allocation",
                "VK_EXT_extended_dynamic_state2",
                "VK_KHR_draw_indirect_count",
                "VK_KHR_bind_memory2",
                "VK_KHR_draw_indirect_count",
                "VK_KHR_get_memory_requirements2",
                "VK_EXT_shader_viewport_index_layer",

                "VK_EXT_scalar_block_layout",
                "VK_KHR_depth_stencil_resolve",
                "VK_KHR_descriptor_update_template",
                "VK_KHR_dynamic_rendering_local_read",
                "VK_KHR_external_memory",
                "VK_KHR_line_rasterization",
                "VK_KHR_image_format_list",
                "VK_KHR_uniform_buffer_standard_layout",
                "VK_EXT_buffer_device_address",
                "VK_EXT_scalar_block_layout",
                "VK_EXT_image_sliced_view_of_3d",
                "VK_EXT_dynamic_rendering_unused_attachments",
                "VK_EXT_tooling_info",
                "VK_NV_copy_memory_indirect",
                "VK_NV_present_metering",
                "VK_NV_low_latency",
                "VK_NV_dedicated_allocation",
                "VK_EXT_subgroup_size_control",
                "VK_NV_low_latency2",
                "VK_NVX_image_view_handle",
                "VK_EXT_separate_stencil_usage",
                "VK_EXT_index_type_uint8",
                "VK_EXT_dynamic_rendering_unused_attachments",
            };

            const auto available_extensions = _physical_device.enumerateDeviceExtensionProperties();
            for (const auto &[name, _] : available_extensions) {
                if (desired_extensions.contains(name.data())) {
                    const char* s = name.data();
                    extensions.push_back(s);
                }
            }


            vk::PhysicalDeviceFeatures2 f2{};
            f2.features.geometryShader     = true;
            f2.features.tessellationShader = true;

            vk::PhysicalDeviceVulkan11Features v11f{};
            vk::PhysicalDeviceVulkan12Features v12f{};
            vk::PhysicalDeviceVulkan13Features v13f{};
            v13f.dynamicRendering   = true;
            v13f.inlineUniformBlock = true;
            v13f.synchronization2   = true;

            vk::PhysicalDeviceVulkan14Features v14f{};
            v14f.pushDescriptor = true;

            f2.pNext   = &v11f;
            v11f.pNext = &v12f;
            v12f.pNext = &v13f;
            v13f.pNext = &v14f;

            create_info.setPEnabledExtensionNames(extensions);
            create_info.setQueueCreateInfos(queue_create_infos);
            create_info.pNext = &f2;

            _device = vk::raii::Device(_physical_device, create_info);

            _graphics_queue = _device.getQueue(_graphics_family, 0);
            _transfer_queue = _device.getQueue(_transfer_family, 0);
            _present_queue  = _device.getQueue(_present_family, 0);
            _compute_queue  = _device.getQueue(_compute_family, 0);

            _enabled_extensions = extensions;
            for (const auto &ext : _enabled_extensions) {
                _enabled_extension_set.insert(ext);
            }
        }
    }

    RenderContext::~RenderContext() {}

    const vk::raii::Instance &RenderContext::instance() {
        return _instance;
    }

    const vk::raii::PhysicalDevice &RenderContext::physical_device() {
        return _physical_device;
    }

    const vk::raii::Device &RenderContext::device() {
        return _device;
    }

    const vk::raii::Queue &RenderContext::graphics_queue() {
        return _graphics_queue;
    }

    const vk::raii::Queue &RenderContext::present_queue() {
        return _present_queue;
    }

    const vk::raii::Queue &RenderContext::transfer_queue() {
        return _transfer_queue;
    }

    const vk::raii::Queue &RenderContext::compute_queue() {
        return _compute_queue;
    }

    uint32_t RenderContext::graphics_family() {
        return _graphics_family;
    }

    uint32_t RenderContext::present_family() {
        return _present_family;
    }

    uint32_t RenderContext::transfer_family() {
        return _transfer_family;
    }

    uint32_t RenderContext::compute_family() {
        return _compute_family;
    }

    std::vector<const char *> RenderContext::enabled_extensions() {
        return _enabled_extensions;
    }

    bool RenderContext::is_extension_enabled(const std::string_view extension) {
        return _enabled_extension_set.contains(std::string(extension));
    }

    std::tuple<std::shared_ptr<api::IWindow>, std::shared_ptr<api::ISwapchain>> RenderContext::create_window(std::string_view title, const vk::Extent2D &size) {
        auto p = std::make_shared<Window>(title, size.width, size.height, std::reinterpret_pointer_cast<RenderContext>(shared_from_this()));
        return std::make_tuple<std::shared_ptr<api::IWindow>, std::shared_ptr<api::ISwapchain>>(p, p);
    }
} // namespace neuron
