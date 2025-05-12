#include <array>
#include <format>
#include <iostream>
#include <span>
#include <string_view>
#include <sys/prctl.h>
#include <ranges>
#include <unistd.h>

// Constants as constexpr
constexpr uint32_t KERNEL_SU_OPTION = 0xDEADBEEF;

// Command definitions using enum class for better type safety
enum class SusCommand : uint32_t {
    SHOW_VERSION = 0x555e1,
    SHOW_ENABLED_FEATURES = 0x555e2,
    SHOW_VARIANT = 0x555e3,
    SHOW_SUS_SU_WORKING_MODE = 0x555e4,
    IS_SUS_SU_READY = 0x555f0,
    SUS_SU = 0x60000
};

// SUS_SU modes
enum class SuMode : int {
    DISABLED = 0,
    WITH_HOOKS = 2
};

// Feature flags using enum class with scoped enums
enum class Feature : unsigned long {
    SUS_PATH = 1UL << 0,
    SUS_MOUNT = 1UL << 1,
    AUTO_ADD_SUS_KSU_DEFAULT_MOUNT = 1UL << 2,
    AUTO_ADD_SUS_BIND_MOUNT = 1UL << 3,
    SUS_KSTAT = 1UL << 4,
    SUS_OVERLAYFS = 1UL << 5,
    TRY_UMOUNT = 1UL << 6,
    AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT = 1UL << 7,
    SPOOF_UNAME = 1UL << 8,
    ENABLE_LOG = 1UL << 9,
    HIDE_KSU_SUSFS_SYMBOLS = 1UL << 10,
    SPOOF_BOOTCONFIG = 1UL << 11,
    OPEN_REDIRECT = 1UL << 12,
    SUS_SU = 1UL << 13
};

// Modern C++ class with RAII
class SusSu {
public:
    explicit constexpr SusSu(SuMode mode) noexcept : mode_(static_cast<int>(mode)) {}
    [[nodiscard]] constexpr auto get_mode() const noexcept { return mode_; }

private:
    int mode_;
};

// Modern C++ class for managing kernel operations
class KernelSuManager {
public:
    [[nodiscard]] static bool enable_sus_su(SuMode last_mode, SuMode target_mode) noexcept {
        SusSu info{target_mode};
        int error = -1;
        prctl(KERNEL_SU_OPTION, static_cast<uint32_t>(SusCommand::SUS_SU), &info, nullptr, &error);
        if (!error) {
            std::cout << std::format("[+] sus_su mode {} is enabled\n", static_cast<int>(target_mode));
        }
        return error == 0;
    }

    static void print_features(unsigned long enabled_features) noexcept {
        constexpr std::array feature_configs{
            std::pair{Feature::SUS_PATH, "CONFIG_KSU_SUSFS_SUS_PATH"},
            std::pair{Feature::SUS_MOUNT, "CONFIG_KSU_SUSFS_SUS_MOUNT"},
            std::pair{Feature::AUTO_ADD_SUS_KSU_DEFAULT_MOUNT, "CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT"},
            std::pair{Feature::AUTO_ADD_SUS_BIND_MOUNT, "CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT"},
            std::pair{Feature::SUS_KSTAT, "CONFIG_KSU_SUSFS_SUS_KSTAT"},
            std::pair{Feature::SUS_OVERLAYFS, "CONFIG_KSU_SUSFS_SUS_OVERLAYFS"},
            std::pair{Feature::TRY_UMOUNT, "CONFIG_KSU_SUSFS_TRY_UMOUNT"},
            std::pair{Feature::AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT, "CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT"},
            std::pair{Feature::SPOOF_UNAME, "CONFIG_KSU_SUSFS_SPOOF_UNAME"},
            std::pair{Feature::ENABLE_LOG, "CONFIG_KSU_SUSFS_ENABLE_LOG"},
            std::pair{Feature::HIDE_KSU_SUSFS_SYMBOLS, "CONFIG_KSU_SUSFS_HIDE_KSU_SUSFS_SYMBOLS"},
            std::pair{Feature::SPOOF_BOOTCONFIG, "CONFIG_KSU_SUSFS_SPOOF_BOOTCONFIG"},
            std::pair{Feature::OPEN_REDIRECT, "CONFIG_KSU_SUSFS_OPEN_REDIRECT"},
            std::pair{Feature::SUS_SU, "CONFIG_KSU_SUSFS_SUS_SU"}
        };

        for (const auto& [feature, config_name] : feature_configs) {
            if (is_feature_enabled(enabled_features, feature)) {
                std::cout << std::format("{}\n", config_name);
            }
        }
    }

    [[nodiscard]] static constexpr bool is_feature_enabled(unsigned long enabled_features, Feature feature) noexcept {
        return (enabled_features & static_cast<unsigned long>(feature)) != 0;
    }

    [[nodiscard]] static bool get_sus_su_working_mode(SuMode& mode) noexcept {
        int temp_mode;
        int error = -1;
        prctl(KERNEL_SU_OPTION, static_cast<uint32_t>(SusCommand::SHOW_SUS_SU_WORKING_MODE), &temp_mode, nullptr, &error);
        if (error == 0) {
            mode = static_cast<SuMode>(temp_mode);
        }
        return error == 0;
    }
};

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << std::format("Usage: {} <support|version|variant|features|sus_su <0|2|mode>>\n", argv[0]);
        return 1;
    }

    std::string_view command{argv[1]};
    int error = -1;

    if (command == "version") {
        std::array<char, 16> version{};
        prctl(KERNEL_SU_OPTION, static_cast<uint32_t>(SusCommand::SHOW_VERSION), version.data(), nullptr, &error);
        std::cout << (error ? "Invalid" : version.data()) << '\n';
    } 
    else if (command == "variant") {
        std::array<char, 16> variant{};
        prctl(KERNEL_SU_OPTION, static_cast<uint32_t>(SusCommand::SHOW_VARIANT), variant.data(), nullptr, &error);
        std::cout << (error ? "Invalid" : variant.data()) << '\n';
    }
    else if (command == "features") {
        unsigned long enabled_features;
        prctl(KERNEL_SU_OPTION, static_cast<uint32_t>(SusCommand::SHOW_ENABLED_FEATURES), &enabled_features, nullptr, &error);
        if (!error) {
            KernelSuManager::print_features(enabled_features);
        } else {
            std::cout << "Invalid\n";
        }
    }
    else if (command == "support") {
        unsigned long enabled_features;
        prctl(KERNEL_SU_OPTION, static_cast<uint32_t>(SusCommand::SHOW_ENABLED_FEATURES), &enabled_features, nullptr, &error);
        std::cout << (error || !enabled_features ? "Unsupported" : "Supported") << '\n';
    }
    else if (argc == 3 && command == "sus_su") {
        std::string_view mode_arg{argv[2]};
        SuMode last_working_mode;
        
        if (!KernelSuManager::get_sus_su_working_mode(last_working_mode)) {
            return 1;
        }

        if (mode_arg == "mode") {
            std::cout << std::format("{}\n", static_cast<int>(last_working_mode));
            return 0;
        }

        try {
            auto target_mode = static_cast<SuMode>(std::stoi(std::string{mode_arg}));
            
            if (target_mode == SuMode::WITH_HOOKS) {
                bool is_sus_su_ready;
                prctl(KERNEL_SU_OPTION, static_cast<uint32_t>(SusCommand::IS_SUS_SU_READY), &is_sus_su_ready, nullptr, &error);
                if (error || !is_sus_su_ready) {
                    std::cout << std::format("[-] sus_su mode {} must be run during or after service stage\n", 
                        static_cast<int>(SuMode::WITH_HOOKS));
                    return 1;
                }
                if (last_working_mode == SuMode::WITH_HOOKS) {
                    std::cout << std::format("[-] sus_su is already in mode {}\n", 
                        static_cast<int>(last_working_mode));
                    return 1;
                }
                KernelSuManager::enable_sus_su(last_working_mode, SuMode::WITH_HOOKS);
            }
            else if (target_mode == SuMode::DISABLED) {
                if (last_working_mode == SuMode::DISABLED) {
                    std::cout << std::format("[-] sus_su is already in mode {}\n", 
                        static_cast<int>(last_working_mode));
                    return 1;
                }
                KernelSuManager::enable_sus_su(last_working_mode, SuMode::DISABLED);
            }
            else {
                std::cerr << std::format("Invalid mode: {}\n", static_cast<int>(target_mode));
                return 1;
            }
        }
        catch (const std::exception&) {
            std::cerr << std::format("Invalid argument: {}\n", mode_arg);
            return 1;
        }
    }
    else {
        std::cerr << std::format("Invalid argument: {}\n", command);
        return 1;
    }

    return 0;
}
