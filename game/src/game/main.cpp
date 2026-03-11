//
// Created by andy on 3/8/26.
//

#include <iostream>
#include <neuron/neuron.hpp>
#include <ranges>

#if defined(__linux__)
#  include <sys/ioctl.h>
#  include <unistd.h>
#elif defined(WIN32)
#else
#  error "Target Unsupported"
#endif

constexpr std::string_view MODULES_DIR = "./modules/";

void module_loader(const std::shared_ptr<neuron::Engine> &engine) {
    if (std::filesystem::exists(MODULES_DIR)) {
        for (auto it = std::filesystem::directory_iterator(MODULES_DIR); it != std::filesystem::directory_iterator(); ++it) {
            engine->load_module(std::filesystem::absolute(it->path()));
        }
    }

    for (const auto &module : engine->get_loaded_modules() | std::views::values) {
        module->entry_point()(engine.get(), {module->path()});
    }
}

constexpr static uint32_t MAX_COLS = 8;

struct boxinfo_t {
    std::string color;
    std::string caption_color;
};

const boxinfo_t default_boxinfo{"34", "94"};

void print_boxed(const char *caption, const std::string &text, const boxinfo_t &boxinfo = default_boxinfo) {
    std::vector<std::string> lines;
    std::stringstream        ss(text);
    std::string              token;
    std::size_t              longest = strlen(caption);
    while (std::getline(ss, token, '\n')) {
        longest = std::max(longest, token.size());
        lines.push_back(token);
    }

    std::print(std::cout, "\033[{}m\u256d\u2500\033[{}m{}\033[{}m", boxinfo.color, boxinfo.caption_color, caption, boxinfo.color);
    for (uint32_t i = 0; i < longest + 1 - strlen(caption); i++) {
        std::cout << "\u2500";
    }
    std::cout << "\u256e\n";
    for (const auto &line : lines) {
        std::println(std::cout, "\033[{}m\u2502\033[0m {} \033[{}G\033[{}m\u2502\033[0m", boxinfo.color, line, longest + 4, boxinfo.color);
    }

    std::print(std::cout, "\033[{}m\u2570", boxinfo.color);
    for (uint32_t i = 0; i < longest + 2; i++) {
        std::cout << "\u2500";
    }
    std::cout << "\u256f\033[0m\n";
}

template <std::ranges::sized_range R, std::invocable<const std::ranges::range_value_t<R> &> C>
void print_table(const char *caption, R &&range, C transformer)
    requires(std::same_as<decltype(std::declval<C>()(std::declval<const std::ranges::range_value_t<R> &>())), std::string>)
{
    std::vector<std::string> strings;
    strings.reserve(std::ranges::size(range));
    std::size_t longest = strlen(caption);
    for (const auto &value : range) {
        std::string s = transformer(value);
        strings.push_back(s);
        longest = std::max(longest, s.size());
    }

    int term_cols = 80;

#if defined(__linux__)
    winsize w{};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    term_cols = w.ws_col;
#elif defined(WIN32)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        term_cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
#endif

    uint32_t ncol   = (term_cols - 1) / (longest + 3);
    uint32_t cwidth = longest + 3;

    ncol = std::min(MAX_COLS, ncol);
    ncol = std::min<uint32_t>(ncol, std::ranges::size(range));

    std::cout << "\033[34m\u256d\u2500\033[94m" << caption << "\033[34m";
    uint32_t fcol_lines = cwidth - 2 - strlen(caption);
    for (uint32_t i = 0; i < fcol_lines; ++i) {
        std::cout << "\u2500";
    }
    for (uint32_t i = 1; i < ncol; ++i) {
        std::cout << "\u252c";
        for (uint32_t j = 0; j < cwidth - 1; ++j) {
            std::cout << "\u2500";
        }
    }
    std::cout << "\u256e\033[0m\n";

    for (const auto &[i, s] : strings | std::views::enumerate) {
        const uint32_t col  = i % ncol;
        uint32_t       tcol = (col + 1) * cwidth + 1;
        if (col == 0) {
            std::cout << "\033[34m\u2502\033[0m";
        }
        std::print(std::cout, " {} \033[{}G\033[34m\u2502\033[0m", s, tcol);
        if (i == strings.size() - 1) {
            for (uint32_t j = col; j < ncol; ++j) {
                std::print(std::cout, "\033[{}G\033[34m\u2502\033[0m", (j + 1) * cwidth + 1);
            }
            std::println(std::cout);
        } else if (col == ncol - 1) {
            std::println(std::cout);
        }
    }

    std::cout << "\033[34m\u2570";
    for (uint32_t j = 0; j < cwidth - 1; ++j) {
        std::cout << "\u2500";
    }
    for (uint32_t i = 1; i < ncol; ++i) {
        std::cout << "\u2534";
        for (uint32_t j = 0; j < cwidth - 1; ++j) {
            std::cout << "\u2500";
        }
    }
    std::cout << "\u256f\033[0m\n";
}

int main(int argc, char **argv) {
#ifdef WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    const auto engine = neuron::Engine::create();
    module_loader(engine);

    const auto render_context = engine->render_context();
    const auto gpu_props      = render_context->physical_device().getProperties();

    const std::string text = std::format("GPU: {}\n"
                                         "Graphics Family: {}\n"
                                         "Transfer Family: {}\n"
                                         "Compute Family: {}\n"
                                         "Present Family: {}",
                                         gpu_props.deviceName.data(), render_context->graphics_family(), render_context->transfer_family(), render_context->compute_family(),
                                         render_context->present_family());

    int i[1] = {0};
    print_boxed("GPU Info", text, {"31", "91"});

    const auto extensions = render_context->physical_device().enumerateDeviceExtensionProperties();
    print_table("Extensions", extensions, [&](const vk::ExtensionProperties &props) -> std::string {
        if (render_context->is_extension_enabled(props.extensionName.data())) {
            return std::format("\033[1;92m{}\033[0m", props.extensionName.data());
        }
        return props.extensionName.data();
    });

    auto [window, swapchain] = render_context->create_window("Hello!", {800, 600});

    std::string text2 =
        std::format("# of Images: {}\nFormat: {}\nColor Space: {}\nPresent Mode: {}\nExtent: {}x{}", swapchain->images().size(), vk::to_string(swapchain->surface_format().format),
                    vk::to_string(swapchain->surface_format().colorSpace), vk::to_string(swapchain->present_mode()), swapchain->extent().width, swapchain->extent().height);

    print_boxed("Swapchain", text2, {"33", "93"});

    while (!window->should_close()) {
        window->update_events();
    }

    return 0;
}
