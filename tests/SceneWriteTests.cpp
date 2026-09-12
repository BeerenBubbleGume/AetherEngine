#include "SceneWriteTestSupport.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

using namespace scene_write_tests;

namespace {
void createsAndReplaces() {
    Fixture fixture;
    struct RestoreWorkingDirectory {
        fs::path previous = fs::current_path();
        ~RestoreWorkingDirectory() { fs::current_path(previous); }
    } restore;
    fs::current_path(fixture.base);
    const std::string binary{"a\0b\xff\r\n", 6};
    auto result = fixture.write(binary, binary.size());
    require(result.has_value(), result ? "" : result.error().message);
    require(get(fixture.destination) == binary, "New file must contain all bytes at the size limit");
    const auto expectedEntries = entries(fixture.base);
    for (const auto& contents : {std::string("short"), std::string(1024, 'x'), std::string("z")}) {
        result = fixture.write(contents);
        require(result.has_value(), result ? "" : result.error().message);
        require(get(fixture.destination) == contents, "Replacement has missing bytes or a stale suffix");
        require(entries(fixture.base) == expectedEntries, "A successful save left extra files");
    }
}

void rejectsInvalidRequests() {
    Fixture fixture;
    put(fixture.destination, "original");
    const auto outside = fixture.base / "outside.scene.json";
    put(outside, "outside original");
    fs::create_directory(fixture.scenes / "directory.scene.json");
    const auto expectedEntries = entries(fixture.base);
    require(!fixture.write(""), "Empty data must be rejected");
    require(!fixture.write("too large", 3), "Oversized data must be rejected");
    require(!fixture.write("", 4096, "assets/scenes/new.scene.json"), "Empty new file must be rejected");
    require(!fixture.write("too large", 3, "assets/scenes/new.scene.json"), "Oversized new file must be rejected");
    require(!fixture.write("new", 4096, "../outside.scene.json"), "Traversal must be rejected");
    require(!fixture.write("new", 4096, outside.generic_string()), "Outside absolute path must be rejected");
    require(!fixture.write("new", 4096, "assets/scenes/directory.scene.json"), "Directory must be rejected");
    require(!fixture.write("new", 4096, "missing/scene.json"), "Missing parent must be rejected");
    require(get(fixture.destination) == "original", "Invalid request changed the existing scene");
    require(get(outside) == "outside original", "Invalid request changed an outside file");
    require(entries(fixture.base) == expectedEntries, "Invalid request created or removed files");
}

void rejectsSymlinks() {
    Fixture fixture;
    const auto outside = fixture.base / "outside.scene.json";
    put(outside, "outside original");
    std::error_code error;
    fs::create_symlink(outside, fixture.destination, error);
    if (error) {
        std::cout << "SKIP: file symlink creation unavailable: " << error.message() << '\n';
    } else {
        const auto before = entries(fixture.root);
        require(!fixture.write("new"), "Destination symlink must be rejected");
        require(fs::is_symlink(fs::symlink_status(fixture.destination)), "Destination symlink was replaced");
        require(entries(fixture.root) == before, "Rejected symlink write left extra files");
    }
    fs::create_directories(fixture.base / "outside scenes");
    put(fixture.base / "outside scenes" / "Main.scene.json", "parent original");
    fs::create_directory_symlink(fixture.base / "outside scenes", fixture.root / "linked", error);
    if (error) {
        std::cout << "SKIP: directory symlink creation unavailable: " << error.message() << '\n';
    } else {
        const auto before = entries(fixture.base);
        require(!fixture.write("new", 4096, "linked/Main.scene.json"), "Symlink parent must be rejected");
        require(entries(fixture.base) == before, "Rejected parent symlink write left extra files");
    }
    require(get(outside) == "outside original", "Symlink write changed outside contents");
    require(get(fixture.base / "outside scenes" / "Main.scene.json") == "parent original",
            "Parent symlink write changed outside contents");
}

#ifdef _WIN32
struct HeldFile {
    HANDLE handle;
    HeldFile(const fs::path& path, DWORD sharing)
        : handle(CreateFileW(path.c_str(), GENERIC_READ, sharing, nullptr, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, nullptr)) {
        require(handle != INVALID_HANDLE_VALUE, "Could not open the old scene for the test");
    }
    ~HeldFile() { CloseHandle(handle); }
};

void replacesFileInsteadOfTruncating() {
    Fixture fixture;
    put(fixture.destination, "original scene");
    const auto before = entries(fixture.base);
    const auto fileInformation = [&]() {
        HeldFile file(fixture.destination, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
        BY_HANDLE_FILE_INFORMATION information{};
        require(GetFileInformationByHandle(file.handle, &information) != FALSE,
                "Could not obtain scene file identity");
        return information;
    };
    // MoveFileExW need not replace an open destination even with SHARE_DELETE.
    // Close the inspection handle before saving and compare filesystem identity:
    // truncating the existing file would keep its volume/file-index tuple.
    const auto old = fileInformation();
    const auto result = fixture.write("replacement");
    require(result.has_value(), result ? "" : result.error().message);
    require(get(fixture.destination) == "replacement", "Path does not point to the new scene");
    const auto replacement = fileInformation();
    const bool sameFile = old.dwVolumeSerialNumber == replacement.dwVolumeSerialNumber &&
        old.nFileIndexHigh == replacement.nFileIndexHigh && old.nFileIndexLow == replacement.nFileIndexLow;
    require(!sameFile, "Save modified the old file in place instead of replacing it");
    require(entries(fixture.base) == before, "Replacement left extra files");
}

void preservesFileWhenReplacementIsBlocked() {
    Fixture fixture;
    put(fixture.destination, "original scene");
    const auto before = entries(fixture.base);
    {
        HeldFile old(fixture.destination, FILE_SHARE_READ | FILE_SHARE_WRITE);
        const auto result = fixture.write("replacement");
        require(!result, "Replacement must fail while an open reader denies deletion");
        require(get(fixture.destination) == "original scene", "Failed replacement modified the old scene");
        require(entries(fixture.base) == before, "Failed replacement left a temporary file");
    }
    require(fixture.write("replacement").has_value(), "Save must succeed after the reader releases the file");
    require(get(fixture.destination) == "replacement", "Retry saved incorrect contents");
    require(entries(fixture.base) == before, "Retry left a temporary file");
}
#endif
}

int main() {
    int failures = 0;
    failures += run("create binary file and replace shorter/longer from another working directory", createsAndReplaces);
    failures += run("invalid requests preserve files and directories", rejectsInvalidRequests);
    failures += run("symlink destinations and parents", rejectsSymlinks);
#ifdef _WIN32
    failures += run("replacement changes file identity instead of truncating in place", replacesFileInsteadOfTruncating);
    failures += run("blocked replacement preserves scene, cleans up, and permits retry", preservesFileWhenReplacementIsBlocked);
#endif
    return failures == 0 ? 0 : 1;
}
