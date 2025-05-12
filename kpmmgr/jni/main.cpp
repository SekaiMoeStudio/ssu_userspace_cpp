#include <unistd.h>
#include <sys/prctl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

namespace ksu {
    // 基础常量定义
    enum : uint32_t {
        OPTIONS = 0xdeadbeef
    };

    // 命令枚举
    enum Command {
        KPM_LOAD = 28,
        KPM_UNLOAD = 29,
        KPM_NUM = 30,
        KPM_LIST = 31,
        KPM_INFO = 32,
        KPM_CONTROL = 33,
        KPM_VERSION = 34
    };

    inline int control_code(Command cmd) {
        return static_cast<int>(cmd);
    }
}

// KPM管理类
struct KpmManager {
    static void print_usage(const char* prog) {
        printf("Usage: %s <command> [args]\n"
               "Commands:\n"
               "  load <path> <args>    Load a KPM module\n"
               "  unload <name>         Unload a KPM module\n"
               "  num                   Get number of loaded modules\n"
               "  list                  List loaded KPM modules\n"
               "  info <name>           Get info of a KPM module\n"
               "  control <name> <args> Send control command to a KPM module\n"
               "  version               Print KPM Loader version\n",
               prog);
    }

    static bool load_module(const char* path, const char* args = nullptr) {
        int out = -1;
        prctl(ksu::OPTIONS, ksu::control_code(ksu::KPM_LOAD), path, args, &out);
        if (out > 0) {
            printf("Success\n");
        }
        return handle_result(out);
    }

    static bool unload_module(const char* name) {
        int out = -1;
        prctl(ksu::OPTIONS, ksu::control_code(ksu::KPM_UNLOAD), name, nullptr, &out);
        return handle_result(out);
    }

    static int get_module_count() {
        int out = -1;
        prctl(ksu::OPTIONS, ksu::control_code(ksu::KPM_NUM), nullptr, nullptr, &out);
        if (out >= 0) {
            printf("%d\n", out);
        }
        return out;
    }

    static bool list_modules() {
        char buffer[1024] = {0};
        int out = -1;
        prctl(ksu::OPTIONS, ksu::control_code(ksu::KPM_LIST), buffer, sizeof(buffer), &out);
        if (out >= 0) {
            printf("%s", buffer);
        }
        return handle_result(out);
    }

    static bool get_module_info(const char* name) {
        char buffer[256] = {0};
        int out = -1;
        prctl(ksu::OPTIONS, ksu::control_code(ksu::KPM_INFO), name, buffer, &out);
        if (out >= 0) {
            printf("%s\n", buffer);
        }
        return handle_result(out);
    }

    static bool control_module(const char* name, const char* args) {
        int out = -1;
        prctl(ksu::OPTIONS, ksu::control_code(ksu::KPM_CONTROL), name, args, &out);
        return handle_result(out);
    }

    static bool get_version() {
        char buffer[1024] = {0};
        int out = -1;
        prctl(ksu::OPTIONS, ksu::control_code(ksu::KPM_VERSION), buffer, sizeof(buffer), &out);
        if (out >= 0) {
            printf("%s", buffer);
        }
        return handle_result(out);
    }

private:
    static bool handle_result(int result) {
        if (result < 0) {
            fprintf(stderr, "Error: %s\n", strerror(-result));
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

    if (strcmp(argv[1], "load") == 0 && argc >= 3) {
        return KpmManager::load_module(argv[2], argc > 3 ? argv[3] : nullptr) ? 0 : 1;
    }
    
    if (strcmp(argv[1], "unload") == 0 && argc >= 3) {
        return KpmManager::unload_module(argv[2]) ? 0 : 1;
    }
    
    if (strcmp(argv[1], "num") == 0) {
        const int count = KpmManager::get_module_count();
        return count >= 0 ? 0 : 1;
    }
    
    if (strcmp(argv[1], "list") == 0) {
        return KpmManager::list_modules() ? 0 : 1;
    }
    
    if (strcmp(argv[1], "info") == 0 && argc >= 3) {
        return KpmManager::get_module_info(argv[2]) ? 0 : 1;
    }
    
    if (strcmp(argv[1], "control") == 0 && argc >= 4) {
        return KpmManager::control_module(argv[2], argv[3]) ? 0 : 1;
    }
    
    if (strcmp(argv[1], "version") == 0) {
        return KpmManager::get_version() ? 0 : 1;
    }

    KpmManager::print_usage(argv[0]);
    return 1;
}
