#include <array>
#include <format>
#include <iostream>
#include <span>
#include <string_view>
#include <sys/prctl.h>
#include <unistd.h>

// 使用namespace来组织相关常量和枚举
namespace ksu {
    constexpr uint32_t OPTIONS = 0xdeadbeef;

    // 使用enum class替代宏定义
    enum class Command : int {
        KPM_LOAD = 28,
        KPM_UNLOAD = 29,
        KPM_NUM = 30,
        KPM_LIST = 31,
        KPM_INFO = 32,
        KPM_CONTROL = 33,
        KPM_VERSION = 34
    };

    constexpr auto CONTROL_CODE(Command cmd) noexcept {
        return static_cast<int>(cmd);
    }
}

// KPM管理类
class KpmManager {
public:
    static void print_usage(std::string_view prog) noexcept {
        const auto usage = std::format(
            "Usage: {} <command> [args]\n"
            "Commands:\n"
            "  load <path> <args>    Load a KPM module\n"
            "  unload <name>         Unload a KPM module\n"
            "  num                   Get number of loaded modules\n"
            "  list                  List loaded KPM modules\n"
            "  info <name>           Get info of a KPM module\n"
            "  control <name> <args> Send control command to a KPM module\n"
            "  version               Print KPM Loader version\n",
            prog);
        std::cout << usage;
    }

    [[nodiscard]] static bool load_module(std::string_view path, std::string_view args = {}) noexcept {
        int out = -1;
        const auto ret = prctl(ksu::OPTIONS, 
                             ksu::CONTROL_CODE(ksu::Command::KPM_LOAD),
                             path.data(),
                             args.empty() ? nullptr : args.data(),
                             &out);
        if (out > 0) {
            std::cout << "Success\n";
        }
        return handle_result(out);
    }

    [[nodiscard]] static bool unload_module(std::string_view name) noexcept {
        int out = -1;
        const auto ret = prctl(ksu::OPTIONS,
                             ksu::CONTROL_CODE(ksu::Command::KPM_UNLOAD),
                             name.data(),
                             nullptr,
                             &out);
        return handle_result(out);
    }

    [[nodiscard]] static int get_module_count() noexcept {
        int out = -1;
        const auto ret = prctl(ksu::OPTIONS,
                             ksu::CONTROL_CODE(ksu::Command::KPM_NUM),
                             nullptr,
                             nullptr,
                             &out);
        if (out >= 0) {
            std::cout << out << '\n';
        }
        return out;
    }

    [[nodiscard]] static bool list_modules() noexcept {
        std::array<char, 1024> buffer{};
        int out = -1;
        const auto ret = prctl(ksu::OPTIONS,
                             ksu::CONTROL_CODE(ksu::Command::KPM_LIST),
                             buffer.data(),
                             buffer.size(),
                             &out);
        if (out >= 0) {
            std::cout << buffer.data();
        }
        return handle_result(out);
    }

    [[nodiscard]] static bool get_module_info(std::string_view name) noexcept {
        std::array<char, 256> buffer{};
        int out = -1;
        const auto ret = prctl(ksu::OPTIONS,
                             ksu::CONTROL_CODE(ksu::Command::KPM_INFO),
                             name.data(),
                             buffer.data(),
                             &out);
        if (out >= 0) {
            std::cout << std::format("{}\n", buffer.data());
        }
        return handle_result(out);
    }

    [[nodiscard]] static bool control_module(std::string_view name, std::string_view args) noexcept {
        int out = -1;
        const auto ret = prctl(ksu::OPTIONS,
                             ksu::CONTROL_CODE(ksu::Command::KPM_CONTROL),
                             name.data(),
                             args.data(),
                             &out);
        return handle_result(out);
    }

    [[nodiscard]] static bool get_version() noexcept {
        std::array<char, 1024> buffer{};
        int out = -1;
        const auto ret = prctl(ksu::OPTIONS,
                             ksu::CONTROL_CODE(ksu::Command::KPM_VERSION),
                             buffer.data(),
                             buffer.size(),
                             &out);
        if (out >= 0) {
            std::cout << buffer.data();
        }
        return handle_result(out);
    }

private:
    [[nodiscard]] static bool handle_result(int result) noexcept {
        if (result < 0) {
            std::cerr << std::format("Error: {}\n", strerror(-result));
            return false;
        }
        return true;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        KpmManager::print_usage(argv[0]);
        return 1;
    }

    const std::string_view command{argv[1]};
    
    if (command == "load" && argc >= 3) {
        return KpmManager::load_module(argv[2], argc > 3 ? argv[3] : "") ? 0 : 1;
    }
    
    if (command == "unload" && argc >= 3) {
        return KpmManager::unload_module(argv[2]) ? 0 : 1;
    }
    
    if (command == "num") {
        const auto count = KpmManager::get_module_count();
        return count >= 0 ? 0 : 1;
    }
    
    if (command == "list") {
        return KpmManager::list_modules() ? 0 : 1;
    }
    
    if (command == "info" && argc >= 3) {
        return KpmManager::get_module_info(argv[2]) ? 0 : 1;
    }
    
    if (command == "control" && argc >= 4) {
        return KpmManager::control_module(argv[2], argv[3]) ? 0 : 1;
    }
    
    if (command == "version") {
        return KpmManager::get_version() ? 0 : 1;
    }

    KpmManager::print_usage(argv[0]);
    return 1;
}
