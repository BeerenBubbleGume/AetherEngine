#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "SceneWriteTestSupport.hpp"
#include <algorithm>

// Keep fault injection inside this test executable. The real implementation is
// compiled below with selected WinAPI calls redirected; production has no hooks.
namespace win32_faults {
enum class Mode { none, shortWrites, writeFailure, partialFailure, flushFailure,
                  metadataFailure, collision, collisionsExhausted };
inline Mode mode = Mode::none;
inline unsigned writes = 0;
inline unsigned injected = 0;
inline unsigned creates = 0;
inline std::filesystem::path collisionPath;

BOOL WINAPI writeFile(HANDLE file, LPCVOID data, DWORD count, LPDWORD written, LPOVERLAPPED overlapped) {
    ++writes;
    if (mode == Mode::writeFailure || (mode == Mode::partialFailure && writes > 1)) {
        ++injected;
        *written = 0;
        SetLastError(ERROR_DISK_FULL);
        return FALSE;
    }
    if (mode == Mode::shortWrites || mode == Mode::partialFailure) {
        count = std::min<DWORD>(count, 3);
    }
    return ::WriteFile(file, data, count, written, overlapped);
}

BOOL WINAPI flushFileBuffers(HANDLE file) {
    if (mode == Mode::flushFailure) {
        ++injected;
        SetLastError(ERROR_WRITE_FAULT);
        return FALSE;
    }
    return ::FlushFileBuffers(file);
}

BOOL WINAPI fileInformation(HANDLE file, LPBY_HANDLE_FILE_INFORMATION info) {
    const auto result = ::GetFileInformationByHandle(file, info);
    // Directory validation must still run normally; fail only on the temp file.
    if (result && mode == Mode::metadataFailure && !(info->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        ++injected;
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    return result;
}

BOOL WINAPI deleteFile(LPCWSTR name) {
    const auto result = ::DeleteFileW(name);
    // Successful cleanup must not be relied on to preserve a previous WinAPI
    // error. Deliberately overwrite it to make error-ordering bugs reproducible.
    if (result) {
        SetLastError(ERROR_ACCESS_DENIED);
    }
    return result;
}

HANDLE WINAPI createFile(LPCWSTR name, DWORD access, DWORD sharing, LPSECURITY_ATTRIBUTES security,
                         DWORD disposition, DWORD flags, HANDLE templateFile) {
    if (disposition == CREATE_NEW) {
        ++creates;
        if (mode == Mode::collisionsExhausted) {
            ++injected;
            SetLastError(ERROR_FILE_EXISTS);
            return INVALID_HANDLE_VALUE;
        }
        if (mode == Mode::collision && creates == 1) {
            // Another writer won this exact name. Its contents must survive both
            // the retry and any cleanup performed by the implementation.
            const auto occupied = ::CreateFileW(name, GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                                FILE_ATTRIBUTE_NORMAL, nullptr);
            if (occupied == INVALID_HANDLE_VALUE) {
                return occupied;
            }
            constexpr char sentinel[] = "another writer";
            DWORD written = 0;
            const BOOL saved = ::WriteFile(occupied, sentinel, sizeof(sentinel) - 1, &written, nullptr);
            ::CloseHandle(occupied);
            if (!saved || written != sizeof(sentinel) - 1) {
                SetLastError(ERROR_WRITE_FAULT);
                return INVALID_HANDLE_VALUE;
            }
            collisionPath = name;
            ++injected;
            SetLastError(ERROR_FILE_EXISTS);
            return INVALID_HANDLE_VALUE;
        }
    }
    return ::CreateFileW(name, access, sharing, security, disposition, flags, templateFile);
}
}

#define WriteFile win32_faults::writeFile
#define FlushFileBuffers win32_faults::flushFileBuffers
#define GetFileInformationByHandle win32_faults::fileInformation
#define DeleteFileW win32_faults::deleteFile
#define CreateFileW win32_faults::createFile
#include "../AetherEngine/src/security/PathSecurity.cpp"
#undef CreateFileW
#undef DeleteFileW
#undef GetFileInformationByHandle
#undef FlushFileBuffers
#undef WriteFile

using namespace scene_write_tests;

namespace {
void exercise(win32_faults::Mode mode, bool existingFile) {
    Fixture fixture;
    if (existingFile) {
        put(fixture.destination, "original scene");
    }
    const auto before = entries(fixture.base);
    win32_faults::writes = 0;
    win32_faults::injected = 0;
    win32_faults::creates = 0;
    win32_faults::collisionPath.clear();
    win32_faults::mode = mode;
    const auto result = fixture.write("complete new scene contents");
    win32_faults::mode = win32_faults::Mode::none;

    using enum win32_faults::Mode;
    if (mode == shortWrites || mode == collision) {
        require(result.has_value(), result ? "" : result.error().message);
        require(get(fixture.destination) == "complete new scene contents", "Successful save lost data");
        auto expected = before;
        expected.insert(fixture.destination.lexically_relative(fixture.base));
        if (mode == shortWrites) {
            require(win32_faults::writes > 1, "Test did not exercise repeated short writes");
        } else {
            require(win32_faults::injected == 1 && win32_faults::creates > 1, "Collision was not retried");
            require(get(win32_faults::collisionPath) == "another writer", "Collision damaged another writer's file");
            expected.insert(win32_faults::collisionPath.lexically_relative(fixture.base));
        }
        require(entries(fixture.base) == expected, "Successful save left unexpected temporary files");
        return;
    }

    require(win32_faults::injected > 0, "Requested failure was never injected");
    require(!result, "Injected failure must be reported");
    if (existingFile) {
        require(get(fixture.destination) == "original scene", "Failed save damaged the original file");
    } else {
        require(!fs::exists(fixture.destination), "Failed save published an incomplete new file");
    }
    require(entries(fixture.base) == before, "Failed save left a temporary file behind");
    if (mode == partialFailure) {
        require(win32_faults::writes > 1, "Failure did not occur after a partial write");
    }
    if (mode == collisionsExhausted) {
        require(win32_faults::creates > 1, "Name collisions were not retried");
    }
    if (mode == writeFailure || mode == partialFailure || mode == flushFailure) {
        const DWORD expectedError = mode == flushFailure ? ERROR_WRITE_FAULT : ERROR_DISK_FULL;
        const auto marker = "Windows error " + std::to_string(expectedError);
        require(result.error().message.find(marker) != std::string::npos,
                "Original WinAPI error was lost during cleanup: " + result.error().message);
    }
}
}

int main(int argc, char** argv) {
    using enum win32_faults::Mode;
    const std::pair<std::string_view, win32_faults::Mode> scenarios[] = {
        {"short_writes", shortWrites}, {"write_failure", writeFailure},
        {"partial_failure", partialFailure}, {"flush_failure", flushFailure},
        {"metadata_failure", metadataFailure}, {"collision", collision},
        {"collisions_exhausted", collisionsExhausted}
    };
    if (argc == 2) {
        for (const auto& [name, mode] : scenarios) {
            if (name == argv[1]) {
                int failures = run(std::string(name) + " / existing scene", [=] { exercise(mode, true); });
                failures += run(std::string(name) + " / new scene", [=] { exercise(mode, false); });
                return failures == 0 ? 0 : 1;
            }
        }
    }
    std::cerr << "Expected one known fault scenario name\n";
    return 2;
}
