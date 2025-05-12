#include <cstdint>
#include <span>
#include <system_error>
#include <unistd.h>
#include <sys/prctl.h>

namespace {
    // 使用constexpr定义常量
    constexpr uint32_t KSU_OPTION = 0xdeadbeef;

    // 封装shell执行功能
    class ShellExecutor {
    public:
        [[nodiscard]] static bool launch_shell() noexcept {
            constexpr std::string_view shell_path = "/system/bin/sh";
            return execl(shell_path.data(), shell_path.data(), nullptr) == 0;
        }
    };

    // 封装KernelSU权限提升功能
    class KernelSuManager {
    public:
        [[nodiscard]] static bool request_root() noexcept {
            std::int32_t result = 0;
            return prctl(KSU_OPTION, 0, 0, 0, &result) == 0;
        }
    };
}

// 主函数采用异常安全的方式
[[nodiscard]] int main() {
    try {
        // 请求root权限
        if (!KernelSuManager::request_root()) {
            return EXIT_FAILURE;
        }

        // 启动shell
        if (!ShellExecutor::launch_shell()) {
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        return EXIT_FAILURE;
    }
}
