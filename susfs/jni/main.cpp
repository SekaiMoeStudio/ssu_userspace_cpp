#include <unistd.h>
#include <sys/prctl.h>
#include <stdio.h>
#include <string.h>

namespace ksu {
    enum : uint32_t {
        OPTIONS = 0xdeadbeef
    };

    enum Command : uint32_t {
        SHOW_VERSION = 0x555e1,
        SHOW_ENABLED_FEATURES = 0x555e2,
        SHOW_VARIANT = 0x555e3,
        SHOW_SUS_SU_WORKING_MODE = 0x555e4,
        IS_SUS_SU_READY = 0x555f0,
        SUS_SU = 0x60000
    };

    enum Mode : int {
        DISABLED = 0,
        WITH_HOOKS = 2
    };

    enum Feature : unsigned long {
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
        SUS_SU_FEATURE = 1UL << 13
    };
}

struct SusSu {
    explicit SusSu(int mode) : mode(mode) {}
    int mode;
};

struct KernelSuManager {
    static bool enable_sus_su(int last_working_mode, int target_working_mode) {
        SusSu info(target_working_mode);
        int error = -1;
        prctl(ksu::OPTIONS, static_cast<int>(ksu::Command::SUS_SU), &info, nullptr, &error);
        if (!error) {
            printf("[+] sus_su mode %d is enabled\n", target_working_mode);
        }
        return error == 0;
    }

    static void print_features(unsigned long enabled_features) {
        struct FeatureConfig {
            ksu::Feature feature;
            const char* name;
        };

        static const FeatureConfig configs[] = {
            {ksu::Feature::SUS_PATH, "CONFIG_KSU_SUSFS_SUS_PATH"},
            {ksu::Feature::SUS_MOUNT, "CONFIG_KSU_SUSFS_SUS_MOUNT"},
            {ksu::Feature::AUTO_ADD_SUS_KSU_DEFAULT_MOUNT, "CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT"},
            {ksu::Feature::AUTO_ADD_SUS_BIND_MOUNT, "CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT"},
            {ksu::Feature::SUS_KSTAT, "CONFIG_KSU_SUSFS_SUS_KSTAT"},
            {ksu::Feature::SUS_OVERLAYFS, "CONFIG_KSU_SUSFS_SUS_OVERLAYFS"},
            {ksu::Feature::TRY_UMOUNT, "CONFIG_KSU_SUSFS_TRY_UMOUNT"},
            {ksu::Feature::AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT, "CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT"},
            {ksu::Feature::SPOOF_UNAME, "CONFIG_KSU_SUSFS_SPOOF_UNAME"},
            {ksu::Feature::ENABLE_LOG, "CONFIG_KSU_SUSFS_ENABLE_LOG"},
            {ksu::Feature::HIDE_KSU_SUSFS_SYMBOLS, "CONFIG_KSU_SUSFS_HIDE_KSU_SUSFS_SYMBOLS"},
            {ksu::Feature::SPOOF_BOOTCONFIG, "CONFIG_KSU_SUSFS_SPOOF_BOOTCONFIG"},
            {ksu::Feature::OPEN_REDIRECT, "CONFIG_KSU_SUSFS_OPEN_REDIRECT"},
            {ksu::Feature::SUS_SU_FEATURE, "CONFIG_KSU_SUSFS_SUS_SU"}
        };

        for (const auto& config : configs) {
            if (is_feature_enabled(enabled_features, config.feature)) {
                printf("%s\n", config.name);
            }
        }
    }

    static bool is_feature_enabled(unsigned long enabled_features, ksu::Feature feature) {
        return (enabled_features & static_cast<unsigned long>(feature)) != 0;
    }

    static bool get_sus_su_working_mode(int* mode) {
        int error = -1;
        prctl(ksu::OPTIONS, static_cast<int>(ksu::Command::SHOW_SUS_SU_WORKING_MODE), mode, nullptr, &error);
        return error == 0;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <support|version|variant|features|sus_su <0|2|mode>>\n", argv[0]);
        return 1;
    }

    int error = -1;

    if (strcmp(argv[1], "version") == 0) {
        char version[16] = {0};
        prctl(ksu::OPTIONS, static_cast<int>(ksu::Command::SHOW_VERSION), version, nullptr, &error);
        printf("%s\n", error ? "Invalid" : version);
    } 
    else if (strcmp(argv[1], "variant") == 0) {
        char variant[16] = {0};
        prctl(ksu::OPTIONS, static_cast<int>(ksu::Command::SHOW_VARIANT), variant, nullptr, &error);
        printf("%s\n", error ? "Invalid" : variant);
    }
    else if (strcmp(argv[1], "features") == 0) {
        unsigned long enabled_features;
        prctl(ksu::OPTIONS, static_cast<int>(ksu::Command::SHOW_ENABLED_FEATURES), &enabled_features, nullptr, &error);
        if (!error) {
            KernelSuManager::print_features(enabled_features);
        } else {
            printf("Invalid\n");
        }
    }
    else if (strcmp(argv[1], "support") == 0) {
        unsigned long enabled_features;
        prctl(ksu::OPTIONS, static_cast<int>(ksu::Command::SHOW_ENABLED_FEATURES), &enabled_features, nullptr, &error);
        printf("%s\n", error || !enabled_features ? "Unsupported" : "Supported");
    }

   else if (argc == 3 && strcmp(argv[1], "sus_su") == 0) {
        int last_working_mode;
        if (!KernelSuManager::get_sus_su_working_mode(&last_working_mode)) {
            return 1;
        }

        if (strcmp(argv[2], "mode") == 0) {
            printf("%d\n", last_working_mode);
            return 0;
        }

        char* endptr;
        int target_working_mode = strtol(argv[2], &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "Invalid argument: %s\n", argv[2]);
            return 1;
        }

        if (target_working_mode == static_cast<int>(ksu::Mode::WITH_HOOKS)) {
            bool is_sus_su_ready;
            prctl(ksu::OPTIONS, static_cast<int>(ksu::Command::IS_SUS_SU_READY), &is_sus_su_ready, nullptr, &error);
            if (error || !is_sus_su_ready) {
                printf("[-] sus_su mode %d must be run during or after service stage\n", 
                       static_cast<int>(ksu::Mode::WITH_HOOKS));
                return 1;
            }
            if (last_working_mode == static_cast<int>(ksu::Mode::WITH_HOOKS)) {
                printf("[-] sus_su is already in mode %d\n", last_working_mode);
                return 1;
            }
            KernelSuManager::enable_sus_su(last_working_mode, static_cast<int>(ksu::Mode::WITH_HOOKS));
        }
        else if (target_working_mode == static_cast<int>(ksu::Mode::DISABLED)) {
            if (last_working_mode == static_cast<int>(ksu::Mode::DISABLED)) {
                printf("[-] sus_su is already in mode %d\n", last_working_mode);
                return 1;
            }
            KernelSuManager::enable_sus_su(last_working_mode, static_cast<int>(ksu::Mode::DISABLED));
        }
        else {
            fprintf(stderr, "Invalid mode: %d\n", target_working_mode);
            return 1;
        }
    }
    else {
        fprintf(stderr, "Invalid argument: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
