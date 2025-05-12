#include <unistd.h>
#include <sys/prctl.h>
#include <stdint.h>

namespace {
    constexpr uint32_t KSU_OPTION = 0xdeadbeef;

    class ShellExecutor {
    public:
        static bool launch_shell() noexcept {
            return execl("/system/bin/sh", "sh", nullptr) == 0;
        }
    };

    class KernelSuManager {
    public:
        static bool request_root() noexcept {
            int32_t result = 0;
            return prctl(KSU_OPTION, 0, 0, 0, &result) == 0;
        }
    };
}

int main() {
    if (!KernelSuManager::request_root()) {
        return 1;
    }

    if (!ShellExecutor::launch_shell()) {
        return 1;
    }

    return 0;
}
