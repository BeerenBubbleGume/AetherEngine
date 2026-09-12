#pragma once

#include "security/PathSecurity.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>

namespace scene_write_tests {
namespace fs = std::filesystem;
namespace security = AetherEngine::security;

inline void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

inline void put(const fs::path& path, std::string_view contents) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    stream.close();
    require(!stream.fail(), "Could not write test fixture");
}

inline auto get(const fs::path& path) -> std::string {
    std::ifstream stream(path, std::ios::binary);
    require(stream.is_open(), "Could not read test fixture");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

inline auto entries(const fs::path& directory) -> std::set<fs::path> {
    std::set<fs::path> result;
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        result.insert(entry.path().lexically_relative(directory));
    }
    return result;
}

// Every test owns a newly reserved temporary directory, including its outside-root
// sentinels. Cleanup never visits the repository or another test's fixtures.
struct Fixture {
    fs::path base;
    fs::path root;
    fs::path scenes;
    fs::path destination;

    Fixture() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned attempt = 0; attempt < 100; ++attempt) {
            const auto candidate = fs::temp_directory_path() /
                ("aether-scene-write-" + std::to_string(stamp) + "-" + std::to_string(attempt));
            if (fs::create_directory(candidate)) {
                base = fs::canonical(candidate);
                break;
            }
        }
        require(!base.empty(), "Could not reserve an isolated test directory");
        root = base / fs::path(u8"project \u0422\u0435\u0441\u0442");
        scenes = root / "assets" / "scenes";
        fs::create_directories(scenes);
        root = fs::canonical(root);
        destination = scenes / "Main scene.scene.json";
    }

    Fixture(const Fixture&) = delete;
    auto operator=(const Fixture&) -> Fixture& = delete;
    ~Fixture() {
        std::error_code error;
        fs::remove_all(base, error);
        if (error) {
            std::cerr << "Fixture cleanup failed: " << base << ": " << error.message() << '\n';
        }
    }

    auto write(std::string_view contents, std::uint64_t limit = 4096,
               std::string_view relative = "assets/scenes/Main scene.scene.json") const {
        return security::writeRegularFileWithin(root, relative,
            {reinterpret_cast<const std::uint8_t*>(contents.data()), contents.size()}, limit);
    }
};

template<class Function>
inline int run(std::string_view name, Function function) {
    try {
        function();
        std::cout << "PASS: " << name << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << name << ": " << error.what() << '\n';
        return 1;
    }
}
}
