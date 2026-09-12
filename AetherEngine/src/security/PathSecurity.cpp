#include "security/PathSecurity.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <exception>
#include <iterator>
#include <limits>
#include <new>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <process.h>
#else
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#if defined(__linux__)
#include <linux/openat2.h>
#include <sys/syscall.h>
#endif
#include <unistd.h>
#endif

namespace {
    using AetherEngine::security::PathSecurityError;

    constexpr std::size_t MaxPathLength = 4096;
    constexpr std::size_t MaxSceneNameLength = 128;

    auto containsControlCharacter(std::string_view value) -> bool {
        return std::ranges::any_of(value, [](const unsigned char character) {
            return character < 0x20 || character == 0x7f;
        });
    }

    auto pathComponentEqual(
        const std::filesystem::path& left,
        const std::filesystem::path& right
    ) -> bool {
#if defined(_WIN32)
        const auto& leftNative = left.native();
        const auto& rightNative = right.native();
        if (leftNative.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
            rightNative.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            return false;
        }
        return CompareStringOrdinal(
            leftNative.data(),
            static_cast<int>(leftNative.size()),
            rightNative.data(),
            static_cast<int>(rightNative.size()),
            TRUE
        ) == CSTR_EQUAL;
#else
        return left == right;
#endif
    }

    auto isPathWithin(
        const std::filesystem::path& root,
        const std::filesystem::path& candidate
    ) -> bool {
        auto rootPart = root.begin();
        auto candidatePart = candidate.begin();
        for (; rootPart != root.end() && candidatePart != candidate.end(); ++rootPart, ++candidatePart) {
            if (!pathComponentEqual(*rootPart, *candidatePart)) {
                return false;
            }
        }
        return rootPart == root.end();
    }

    auto pathsEqual(
        const std::filesystem::path& left,
        const std::filesystem::path& right
    ) -> bool {
        return isPathWithin(left, right) && isPathWithin(right, left);
    }

    auto upperAscii(std::string_view value) -> std::string {
        std::string result{value};
        std::ranges::transform(result, result.begin(), [](const unsigned char character) {
            if (character >= 'a' && character <= 'z') {
                return static_cast<char>(character - ('a' - 'A'));
            }
            return static_cast<char>(character);
        });
        return result;
    }

    auto isWindowsDeviceName(std::string_view name) -> bool {
        const auto dot = name.find('.');
        auto stemView = name.substr(0, dot);
        while (!stemView.empty() && (stemView.back() == '.' || stemView.back() == ' ')) {
            stemView.remove_suffix(1);
        }
        const auto stem = upperAscii(stemView);
        constexpr std::array<std::string_view, 7> fixedNames{
            "CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$", "CLOCK$"
        };
        if (std::ranges::find(fixedNames, stem) != fixedNames.end()) {
            return true;
        }
        if (!stem.starts_with("COM") && !stem.starts_with("LPT")) {
            return false;
        }
        const auto suffix = std::string_view{stem}.substr(3);
        return (suffix.size() == 1 && suffix.front() >= '1' && suffix.front() <= '9') ||
            suffix == "\xc2\xb9" || suffix == "\xc2\xb2" || suffix == "\xc2\xb3";
    }

    auto hasUnsafeWindowsPathComponent(std::string_view path) -> bool {
#if defined(_WIN32)
        std::size_t componentStart = 0;
        while (componentStart <= path.size()) {
            const auto separator = path.find_first_of("/\\", componentStart);
            const auto componentEnd = separator == std::string_view::npos ? path.size() : separator;
            const auto component = path.substr(componentStart, componentEnd - componentStart);
            if (!component.empty() &&
                (component.ends_with('.') || component.ends_with(' ') || isWindowsDeviceName(component))) {
                return true;
            }
            if (separator == std::string_view::npos) {
                break;
            }
            componentStart = separator + 1;
        }
#else
        static_cast<void>(path);
#endif
        return false;
    }

    auto lexicalPathWithin(
        const std::filesystem::path& canonicalRoot,
        std::string_view requestedPath
    ) -> std::expected<std::filesystem::path, PathSecurityError> {
        if (requestedPath.empty() || requestedPath.size() > MaxPathLength ||
            containsControlCharacter(requestedPath)) {
            return std::unexpected(PathSecurityError{
                "Path is empty, too long, or contains control characters"
            });
        }
        if (requestedPath.front() == '/' || requestedPath.front() == '\\') {
            return std::unexpected(PathSecurityError{"Only relative paths are allowed"});
        }
        if (requestedPath.find(':') != std::string_view::npos) {
            return std::unexpected(PathSecurityError{
                "Drive, device, and alternate data stream syntax is not allowed"
            });
        }
        if (hasUnsafeWindowsPathComponent(requestedPath)) {
            return std::unexpected(PathSecurityError{
                "Windows device names and trailing dots or spaces are not allowed"
            });
        }

        std::filesystem::path requested;
        try {
            std::u8string utf8Path;
            utf8Path.reserve(requestedPath.size());
            for (const unsigned char byte : requestedPath) {
                utf8Path.push_back(static_cast<char8_t>(byte));
            }
            requested = std::filesystem::path{utf8Path};
        } catch (const std::exception&) {
            return std::unexpected(PathSecurityError{
                "Path is not valid UTF-8 for the current platform"
            });
        }
        if (requested.is_absolute() || requested.has_root_path()) {
            return std::unexpected(PathSecurityError{"Only relative paths are allowed"});
        }

        const auto candidate = (canonicalRoot / requested).lexically_normal();
        if (!isPathWithin(canonicalRoot, candidate)) {
            return std::unexpected(PathSecurityError{"Path escapes the configured root"});
        }
        const auto relative = candidate.lexically_relative(canonicalRoot);
        if (relative.empty() || relative == "." ||
            std::ranges::any_of(relative, [](const auto& component) { return component == ".."; })) {
            return std::unexpected(PathSecurityError{"Path does not identify an entry within the root"});
        }
        return candidate;
    }

#if defined(_WIN32)
    class UniqueHandle {
    public:
        UniqueHandle() = default;
        explicit UniqueHandle(HANDLE handle) : m_handle(handle) {}
        ~UniqueHandle() { reset(); }
        UniqueHandle(const UniqueHandle&) = delete;
        auto operator=(const UniqueHandle&) -> UniqueHandle& = delete;
        UniqueHandle(UniqueHandle&& other) noexcept : m_handle(std::exchange(other.m_handle, INVALID_HANDLE_VALUE)) {}
        auto operator=(UniqueHandle&& other) noexcept -> UniqueHandle& {
            if (this != &other) {
                reset();
                m_handle = std::exchange(other.m_handle, INVALID_HANDLE_VALUE);
            }
            return *this;
        }
        [[nodiscard]] auto get() const -> HANDLE { return m_handle; }
        [[nodiscard]] auto valid() const -> bool {
            return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE;
        }
        auto reset() -> void {
            if (valid()) {
                CloseHandle(m_handle);
            }
            m_handle = INVALID_HANDLE_VALUE;
        }
    private:
        HANDLE m_handle{INVALID_HANDLE_VALUE};
    };

    auto windowsError(std::string_view prefix) -> PathSecurityError {
        return PathSecurityError{
            std::string{prefix} + " (Windows error " + std::to_string(GetLastError()) + ")"
        };
    }

    auto finalPathForHandle(HANDLE handle)
        -> std::expected<std::filesystem::path, PathSecurityError> {
        constexpr DWORD Flags = FILE_NAME_NORMALIZED | VOLUME_NAME_DOS;
        const auto required = GetFinalPathNameByHandleW(handle, nullptr, 0, Flags);
        if (required == 0 || required > MaxPathLength + 32) {
            return std::unexpected(windowsError("Failed to resolve the opened file handle"));
        }
        std::wstring buffer(static_cast<std::size_t>(required) + 1, L'\0');
        const auto written = GetFinalPathNameByHandleW(
            handle,
            buffer.data(),
            static_cast<DWORD>(buffer.size()),
            Flags
        );
        if (written == 0 || written >= buffer.size()) {
            return std::unexpected(windowsError("Failed to resolve the opened file handle"));
        }
        buffer.resize(written);
        if (buffer.starts_with(L"\\\\?\\UNC\\")) {
            buffer = L"\\\\" + buffer.substr(8);
        } else if (buffer.starts_with(L"\\\\?\\")) {
            buffer.erase(0, 4);
        }
        return std::filesystem::path{buffer}.lexically_normal();
    }

    auto openCheckedDirectory(const std::filesystem::path& path)
        -> std::expected<UniqueHandle, PathSecurityError> {
        UniqueHandle handle{CreateFileW(
            path.c_str(),
            FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
            nullptr
        )};
        if (!handle.valid()) {
            return std::unexpected(windowsError("Failed to lock a directory in the trusted path"));
        }
        BY_HANDLE_FILE_INFORMATION information{};
        if (!GetFileInformationByHandle(handle.get(), &information) ||
            (information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ||
            (information.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
            return std::unexpected(PathSecurityError{
                "A directory in the trusted path is not a direct, non-reparse directory"
            });
        }
        const auto finalPath = finalPathForHandle(handle.get());
        if (!finalPath || !pathsEqual(*finalPath, path.lexically_normal())) {
            return std::unexpected(PathSecurityError{
                "An opened directory resolved to an unexpected final path"
            });
        }
        return handle;
    }

    struct DirectoryLocks {
        std::vector<UniqueHandle> handles;
        std::filesystem::path directory;
    };

    auto lockDirectoryChain(
        const std::filesystem::path& canonicalRoot,
        const std::filesystem::path& directory
    ) -> std::expected<DirectoryLocks, PathSecurityError> {
        if (!isPathWithin(canonicalRoot, directory) ||
            canonicalRoot.root_path().empty() ||
            !pathsEqual(canonicalRoot.root_path(), directory.root_path())) {
            return std::unexpected(PathSecurityError{"Directory is outside the trusted root"});
        }

        DirectoryLocks result;
        result.directory = directory.lexically_normal();
        auto current = canonicalRoot.root_path();
        const auto relative = result.directory.lexically_relative(current);
        try {
            for (const auto& component : relative) {
                if (component == ".") {
                    continue;
                }
                current /= component;
                auto handle = openCheckedDirectory(current);
                if (!handle) {
                    return std::unexpected(handle.error());
                }
                result.handles.push_back(std::move(*handle));
            }
        } catch (const std::bad_alloc&) {
            return std::unexpected(PathSecurityError{"Not enough memory to lock the trusted path"});
        }
        if (result.handles.empty()) {
            return std::unexpected(PathSecurityError{"A filesystem root cannot be used as an asset root"});
        }
        return result;
    }

    auto verifyRegularFileByLockedPath(
        const std::filesystem::path& canonicalRoot,
        const std::filesystem::path& candidate
    ) -> std::expected<void, PathSecurityError> {
        auto locks = lockDirectoryChain(canonicalRoot, candidate.parent_path());
        if (!locks) {
            return std::unexpected(locks.error());
        }
        UniqueHandle file{CreateFileW(
            candidate.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT,
            nullptr
        )};
        if (!file.valid()) {
            return std::unexpected(windowsError("Failed to open the trusted file"));
        }
        BY_HANDLE_FILE_INFORMATION information{};
        if (GetFileType(file.get()) != FILE_TYPE_DISK ||
            !GetFileInformationByHandle(file.get(), &information) ||
            (information.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0 ||
            information.nNumberOfLinks != 1) {
            return std::unexpected(PathSecurityError{
                "Path is not a direct, single-link regular disk file"
            });
        }
        const auto finalPath = finalPathForHandle(file.get());
        if (!finalPath || !pathsEqual(*finalPath, candidate)) {
            return std::unexpected(PathSecurityError{
                "The opened file resolved to an unexpected final path"
            });
        }
        return {};
    }

    auto readFileByLockedPath(
        const std::filesystem::path& canonicalRoot,
        const std::filesystem::path& candidate,
        std::uint64_t maximumBytes
    ) -> std::expected<AetherEngine::security::SecureFileContents, PathSecurityError> {
        auto locks = lockDirectoryChain(canonicalRoot, candidate.parent_path());
        if (!locks) {
            return std::unexpected(locks.error());
        }

        UniqueHandle file{CreateFileW(
            candidate.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN,
            nullptr
        )};
        if (!file.valid()) {
            return std::unexpected(windowsError("Failed to open the trusted file"));
        }
        BY_HANDLE_FILE_INFORMATION information{};
        if (GetFileType(file.get()) != FILE_TYPE_DISK ||
            !GetFileInformationByHandle(file.get(), &information) ||
            (information.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0 ||
            information.nNumberOfLinks != 1) {
            return std::unexpected(PathSecurityError{
                "Path is not a direct, single-link regular disk file"
            });
        }
        const auto finalPath = finalPathForHandle(file.get());
        if (!finalPath || !pathsEqual(*finalPath, candidate)) {
            return std::unexpected(PathSecurityError{
                "The opened file resolved to an unexpected final path"
            });
        }
        LARGE_INTEGER fileSize{};
        if (!GetFileSizeEx(file.get(), &fileSize) || fileSize.QuadPart <= 0 ||
            static_cast<std::uint64_t>(fileSize.QuadPart) > maximumBytes ||
            static_cast<std::uint64_t>(fileSize.QuadPart) >
                static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return std::unexpected(PathSecurityError{"File is empty or exceeds the configured size limit"});
        }

        AetherEngine::security::SecureFileContents result;
        result.canonicalPath = candidate;
        try {
            result.bytes.resize(static_cast<std::size_t>(fileSize.QuadPart));
        } catch (const std::bad_alloc&) {
            return std::unexpected(PathSecurityError{"Not enough memory to read the file"});
        }

        std::size_t offset = 0;
        while (offset < result.bytes.size()) {
            const auto count = static_cast<DWORD>(std::min<std::size_t>(
                result.bytes.size() - offset,
                std::numeric_limits<DWORD>::max()
            ));
            DWORD read = 0;
            if (!ReadFile(file.get(), result.bytes.data() + offset, count, &read, nullptr) ||
                read == 0 || read > count) {
                return std::unexpected(windowsError("Failed to read the complete trusted file"));
            }
            offset += read;
        }
        return result;
    }

    auto writeFileByLockedPath(
        const std::filesystem::path& canonicalRoot,
        const std::filesystem::path& candidate,
        std::span<const std::uint8_t> contents
    ) -> std::expected<void, PathSecurityError> {
        auto locks = lockDirectoryChain(canonicalRoot, candidate.parent_path());
        if (!locks) {
            return std::unexpected(locks.error());
        }
        std::error_code ec;
        const auto status = std::filesystem::symlink_status(candidate, ec);

        if (ec && ec != std::errc::no_such_file_or_directory) {
            return std::unexpected(PathSecurityError{
                "Failed to inspect the destination file: " + ec.message()
            });
        }

        if (status.type() != std::filesystem::file_type::regular &&
            status.type() != std::filesystem::file_type::not_found) {
            return std::unexpected(PathSecurityError{
                "Refusing to replace a non-regular scene file"
            });
        }

        static std::atomic_uint64_t temporaryCounter{0};
        std::filesystem::path temporaryName;
        UniqueHandle temporary;
        for (std::uint32_t attempt = 0; attempt < 16; ++attempt) {
            const auto temporaryFilename =
                L"." + candidate.filename().wstring() +
                L".aether-" + std::to_wstring(GetCurrentProcessId()) +
                L"-" + std::to_wstring(
                    temporaryCounter.fetch_add(1, std::memory_order_relaxed)
                ) + L".tmp";

            temporaryName =
                candidate.parent_path() / temporaryFilename;
            temporary = UniqueHandle{CreateFileW(
                temporaryName.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ,
                nullptr,
                CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                nullptr
            )};
            const DWORD error = GetLastError();
            if (temporary.valid()) {
                break;
            }
            if (error != ERROR_FILE_EXISTS) {
                return std::unexpected(windowsError("Failed to create a protected temporary scene file"));
            }
        }
        if (!temporary.valid()) {
            return std::unexpected(PathSecurityError{
                "Could not reserve a temporary scene file name"
            });
        }
        const auto cleanupTemporary = [&]() {
            temporary.reset();
            DeleteFileW(temporaryName.c_str());
        };

        BY_HANDLE_FILE_INFORMATION information{};
        if (GetFileType(temporary.get()) != FILE_TYPE_DISK ||
            !GetFileInformationByHandle(temporary.get(), &information) ||
            (information.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0 ||
            information.nNumberOfLinks != 1) {
            cleanupTemporary();
            return std::unexpected(PathSecurityError{
                "Destination must be a direct regular file with exactly one hard link"
            });
        }
        const auto finalPath = finalPathForHandle(temporary.get());
        if (!finalPath || !pathsEqual(*finalPath, temporaryName)) {
            cleanupTemporary();
            return std::unexpected(PathSecurityError{
                "The opened destination resolved to an unexpected final path"
            });
        }

        std::size_t offset = 0;
        while (offset < contents.size()) {
            const auto count = static_cast<DWORD>(std::min<std::size_t>(
                contents.size() - offset,
                std::numeric_limits<DWORD>::max()
            ));
            DWORD written = 0;
            if (!WriteFile(temporary.get(), contents.data() + offset, count, &written, nullptr) ||
                written == 0 || written > count) {
                auto error = windowsError("Failed to replace the scene file");
                cleanupTemporary();
                return std::unexpected(std::move(error));
            }
            offset += written;
        }
        if (!FlushFileBuffers(temporary.get())) {
            auto error = windowsError("Failed to flush the scene file");
            cleanupTemporary();
            return std::unexpected(std::move(error));
        }
        temporary.reset();
        if (!MoveFileExW(temporaryName.c_str(),candidate.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            auto error = windowsError("Failed to replace the scene file");
            cleanupTemporary();
            return std::unexpected(std::move(error));
        }
        return {};
    }
#else
    class UniqueFd {
    public:
        UniqueFd() = default;
        explicit UniqueFd(int descriptor) : m_descriptor(descriptor) {}
        ~UniqueFd() { reset(); }
        UniqueFd(const UniqueFd&) = delete;
        auto operator=(const UniqueFd&) -> UniqueFd& = delete;
        UniqueFd(UniqueFd&& other) noexcept : m_descriptor(std::exchange(other.m_descriptor, -1)) {}
        auto operator=(UniqueFd&& other) noexcept -> UniqueFd& {
            if (this != &other) {
                reset();
                m_descriptor = std::exchange(other.m_descriptor, -1);
            }
            return *this;
        }
        [[nodiscard]] auto get() const -> int { return m_descriptor; }
        [[nodiscard]] auto valid() const -> bool { return m_descriptor >= 0; }
        auto reset() -> void {
            if (valid()) {
                close(m_descriptor);
            }
            m_descriptor = -1;
        }
    private:
        int m_descriptor{-1};
    };

    auto posixError(std::string_view prefix, int errorCode) -> PathSecurityError {
        return PathSecurityError{std::string{prefix} + ": " + std::strerror(errorCode)};
    }

    auto posixError(std::string_view prefix) -> PathSecurityError {
        return posixError(prefix, errno);
    }

    auto openDirectoryFd(const std::filesystem::path& directory)
        -> std::expected<UniqueFd, PathSecurityError> {
        UniqueFd current{open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC)};
        if (!current.valid()) {
            return std::unexpected(posixError("Failed to open the filesystem root"));
        }
        for (const auto& component : directory.relative_path()) {
            if (component == ".") {
                continue;
            }
            const auto componentString = component.string();
            UniqueFd next{openat(
                current.get(),
                componentString.c_str(),
                O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW
            )};
            if (!next.valid()) {
                return std::unexpected(posixError("Failed to open a direct directory in the trusted path"));
            }
            current = std::move(next);
        }
        return current;
    }

    auto openEntryWithinFd(
        const std::filesystem::path& canonicalRoot,
        const std::filesystem::path& candidate,
        int finalFlags
    ) -> std::expected<UniqueFd, PathSecurityError> {
        if (!isPathWithin(canonicalRoot, candidate)) {
            return std::unexpected(PathSecurityError{"Entry is outside the trusted root"});
        }
        auto trustedRoot = openDirectoryFd(canonicalRoot);
        if (!trustedRoot) {
            return std::unexpected(trustedRoot.error());
        }
        const auto relative = candidate.lexically_relative(canonicalRoot);
        if (relative.empty()) {
            return std::unexpected(PathSecurityError{"Entry does not resolve beneath the trusted root"});
        }

#if defined(__linux__)
        const auto relativeString = relative.string();
        open_how how{};
        how.flags = static_cast<std::uint64_t>(finalFlags | O_CLOEXEC | O_NOFOLLOW);
        how.resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS |
            RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV;
        const auto descriptor = static_cast<int>(syscall(
            SYS_openat2,
            trustedRoot->get(),
            relativeString.c_str(),
            &how,
            sizeof(how)
        ));
        if (descriptor < 0) {
            return std::unexpected(posixError(
                "Failed to open an entry beneath the trusted root without links or mount crossings"
            ));
        }
        return UniqueFd{descriptor};
#else
        struct stat rootInformation{};
        if (fstat(trustedRoot->get(), &rootInformation) != 0) {
            return std::unexpected(posixError("Failed to inspect the trusted root"));
        }

        auto current = std::move(*trustedRoot);
        for (auto component = relative.begin(); component != relative.end(); ++component) {
            const auto nextComponent = std::next(component);
            const bool isFinal = nextComponent == relative.end();
            const auto componentString = component->string();
            UniqueFd next{openat(
                current.get(),
                componentString.c_str(),
                (isFinal ? finalFlags : O_RDONLY | O_DIRECTORY) | O_CLOEXEC | O_NOFOLLOW
            )};
            if (!next.valid()) {
                return std::unexpected(posixError("Failed to open a direct entry in the trusted path"));
            }
            struct stat information{};
            if (fstat(next.get(), &information) != 0 || information.st_dev != rootInformation.st_dev) {
                return std::unexpected(PathSecurityError{
                    "A mount boundary inside the trusted root is not allowed"
                });
            }
            current = std::move(next);
        }
        return current;
#endif
    }

    auto verifyRegularFileByLockedPath(
        const std::filesystem::path& canonicalRoot,
        const std::filesystem::path& candidate
    ) -> std::expected<void, PathSecurityError> {
        if (!isPathWithin(canonicalRoot, candidate)) {
            return std::unexpected(PathSecurityError{"File is outside the trusted root"});
        }
        auto file = openEntryWithinFd(canonicalRoot, candidate, O_RDONLY | O_NONBLOCK);
        if (!file) {
            return std::unexpected(file.error());
        }
        struct stat information{};
        if (fstat(file->get(), &information) != 0 || !S_ISREG(information.st_mode) ||
            information.st_nlink != 1) {
            return std::unexpected(PathSecurityError{
                "Path is not a direct, single-link regular file"
            });
        }
        return {};
    }

    auto readFileByLockedPath(
        const std::filesystem::path& canonicalRoot,
        const std::filesystem::path& candidate,
        std::uint64_t maximumBytes
    ) -> std::expected<AetherEngine::security::SecureFileContents, PathSecurityError> {
        if (!isPathWithin(canonicalRoot, candidate)) {
            return std::unexpected(PathSecurityError{"File is outside the trusted root"});
        }
        auto file = openEntryWithinFd(canonicalRoot, candidate, O_RDONLY | O_NONBLOCK);
        if (!file) {
            return std::unexpected(file.error());
        }
        struct stat information{};
        if (fstat(file->get(), &information) != 0 || !S_ISREG(information.st_mode) ||
            information.st_nlink != 1 ||
            information.st_size <= 0 || static_cast<std::uint64_t>(information.st_size) > maximumBytes ||
            static_cast<std::uint64_t>(information.st_size) >
                static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return std::unexpected(PathSecurityError{
                "File is not regular, is empty, or exceeds the configured size limit"
            });
        }

        AetherEngine::security::SecureFileContents result;
        result.canonicalPath = candidate;
        try {
            result.bytes.resize(static_cast<std::size_t>(information.st_size));
        } catch (const std::bad_alloc&) {
            return std::unexpected(PathSecurityError{"Not enough memory to read the file"});
        }
        std::size_t offset = 0;
        while (offset < result.bytes.size()) {
            const auto count = read(file->get(), result.bytes.data() + offset, result.bytes.size() - offset);
            if (count < 0 && errno == EINTR) {
                continue;
            }
            if (count <= 0) {
                return std::unexpected(posixError("Failed to read the complete trusted file"));
            }
            offset += static_cast<std::size_t>(count);
        }
        return result;
    }

    auto writeFileByLockedPath(
        const std::filesystem::path& canonicalRoot,
        const std::filesystem::path& candidate,
        std::span<const std::uint8_t> contents
    ) -> std::expected<void, PathSecurityError> {
        if (!isPathWithin(canonicalRoot, candidate)) {
            return std::unexpected(PathSecurityError{"File is outside the trusted root"});
        }
        auto parent = openEntryWithinFd(
            canonicalRoot,
            candidate.parent_path(),
            O_RDONLY | O_DIRECTORY
        );
        if (!parent) {
            return std::unexpected(parent.error());
        }
        const auto filename = candidate.filename().string();
        struct stat existing{};
        if (fstatat(parent->get(), filename.c_str(), &existing, AT_SYMLINK_NOFOLLOW) == 0) {
            if (!S_ISREG(existing.st_mode)) {
                return std::unexpected(PathSecurityError{
                    "Refusing to replace a non-regular scene file"
                });
            }
        } else if (errno != ENOENT) {
            return std::unexpected(posixError("Failed to inspect the destination file"));
        }

        static std::atomic_uint64_t temporaryCounter{0};
        std::string temporaryName;
        UniqueFd temporary;
        for (std::uint32_t attempt = 0; attempt < 16; ++attempt) {
            temporaryName = "." + filename + ".aether-" + std::to_string(getpid()) + "-" +
                std::to_string(temporaryCounter.fetch_add(1, std::memory_order_relaxed)) + ".tmp";
            temporary = UniqueFd{openat(
                parent->get(),
                temporaryName.c_str(),
                O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
                S_IRUSR | S_IWUSR
            )};
            if (temporary.valid()) {
                break;
            }
            if (errno != EEXIST) {
                return std::unexpected(posixError("Failed to create a protected temporary scene file"));
            }
        }
        if (!temporary.valid()) {
            return std::unexpected(PathSecurityError{"Could not reserve a temporary scene file name"});
        }

        const auto cleanupTemporary = [&]() {
            temporary.reset();
            unlinkat(parent->get(), temporaryName.c_str(), 0);
        };
        std::size_t offset = 0;
        while (offset < contents.size()) {
            const auto count = write(temporary.get(), contents.data() + offset, contents.size() - offset);
            if (count < 0 && errno == EINTR) {
                continue;
            }
            if (count <= 0) {
                const auto errorCode = errno;
                cleanupTemporary();
                return std::unexpected(posixError("Failed to write the complete scene file", errorCode));
            }
            offset += static_cast<std::size_t>(count);
        }
        if (fsync(temporary.get()) != 0) {
            const auto errorCode = errno;
            cleanupTemporary();
            return std::unexpected(posixError("Failed to flush the scene file", errorCode));
        }
        temporary.reset();
        if (renameat(parent->get(), temporaryName.c_str(), parent->get(), filename.c_str()) != 0) {
            const auto errorCode = errno;
            unlinkat(parent->get(), temporaryName.c_str(), 0);
            return std::unexpected(posixError("Failed to atomically replace the scene file", errorCode));
        }
        if (fsync(parent->get()) != 0) {
            return std::unexpected(posixError("Scene was replaced but its directory could not be flushed"));
        }
        return {};
    }
#endif
}

auto AetherEngine::security::canonicalDirectory(const std::filesystem::path& directory)
    -> std::expected<std::filesystem::path, PathSecurityError> {
    std::error_code error;
    auto absoluteRoot = std::filesystem::absolute(directory, error);
    if (error) {
        return std::unexpected(PathSecurityError{
            "Failed to make asset root absolute: " + error.message()
        });
    }
    absoluteRoot = absoluteRoot.lexically_normal();
    if (absoluteRoot.has_relative_path() && !absoluteRoot.has_filename()) {
        absoluteRoot = absoluteRoot.parent_path();
    }
#if defined(_WIN32)
    if (absoluteRoot.native().starts_with(L"\\\\")) {
        return std::unexpected(PathSecurityError{"UNC asset roots are not allowed"});
    }
#endif
    if (pathsEqual(absoluteRoot, absoluteRoot.root_path())) {
        return std::unexpected(PathSecurityError{"A filesystem root is too broad to be a trusted root"});
    }
#if defined(_WIN32)
    const auto locks = lockDirectoryChain(absoluteRoot, absoluteRoot);
    if (!locks) {
        return std::unexpected(locks.error());
    }
#else
    const auto descriptor = openDirectoryFd(absoluteRoot);
    if (!descriptor) {
        return std::unexpected(descriptor.error());
    }
#endif
    return absoluteRoot;
}

auto AetherEngine::security::normalizeRelativePathWithin(
    const std::filesystem::path& canonicalRoot,
    std::string_view requestedPath
) -> std::expected<std::filesystem::path, PathSecurityError> {
    return lexicalPathWithin(canonicalRoot, requestedPath);
}

auto AetherEngine::security::resolveRegularFileWithin(
    const std::filesystem::path& canonicalRoot,
    std::string_view requestedPath
) -> std::expected<std::filesystem::path, PathSecurityError> {
    const auto candidate = lexicalPathWithin(canonicalRoot, requestedPath);
    if (!candidate) {
        return std::unexpected(candidate.error());
    }
    const auto verified = verifyRegularFileByLockedPath(canonicalRoot, *candidate);
    if (!verified) {
        return std::unexpected(verified.error());
    }
    return *candidate;
}

auto AetherEngine::security::resolveDirectoryWithin(
    const std::filesystem::path& canonicalRoot,
    std::string_view requestedPath
) -> std::expected<std::filesystem::path, PathSecurityError> {
    const auto candidate = lexicalPathWithin(canonicalRoot, requestedPath);
    if (!candidate) {
        return std::unexpected(candidate.error());
    }
#if defined(_WIN32)
    const auto locks = lockDirectoryChain(canonicalRoot, *candidate);
    if (!locks) {
        return std::unexpected(locks.error());
    }
#else
    const auto descriptor = openEntryWithinFd(
        canonicalRoot,
        *candidate,
        O_RDONLY | O_DIRECTORY
    );
    if (!descriptor) {
        return std::unexpected(descriptor.error());
    }
#endif
    return *candidate;
}

auto AetherEngine::security::readRegularFileWithin(
    const std::filesystem::path& canonicalRoot,
    std::string_view requestedPath,
    std::uint64_t maximumBytes
) -> std::expected<SecureFileContents, PathSecurityError> {
    if (maximumBytes == 0) {
        return std::unexpected(PathSecurityError{"File size limit must be positive"});
    }
    const auto candidate = lexicalPathWithin(canonicalRoot, requestedPath);
    if (!candidate) {
        return std::unexpected(candidate.error());
    }
    return readFileByLockedPath(canonicalRoot, *candidate, maximumBytes);
}

auto AetherEngine::security::writeRegularFileWithin(
    const std::filesystem::path& canonicalRoot,
    std::string_view requestedPath,
    std::span<const std::uint8_t> contents,
    std::uint64_t maximumBytes
) -> std::expected<void, PathSecurityError> {
    if (contents.empty() || contents.size() > maximumBytes) {
        return std::unexpected(PathSecurityError{
            "File is empty or exceeds the configured size limit"
        });
    }
    const auto candidate = lexicalPathWithin(canonicalRoot, requestedPath);
    if (!candidate) {
        return std::unexpected(candidate.error());
    }
    return writeFileByLockedPath(canonicalRoot, *candidate, contents);
}

auto AetherEngine::security::isSafeSceneName(std::string_view sceneName) -> bool {
    if (sceneName.empty() || sceneName.size() > MaxSceneNameLength ||
        containsControlCharacter(sceneName)) {
        return false;
    }
    if (sceneName == "." || sceneName == ".." ||
        sceneName.ends_with('.') || sceneName.ends_with(' ')) {
        return false;
    }
    if (sceneName.find_first_of("/\\:*?\"<>|") != std::string_view::npos) {
        return false;
    }
    return !isWindowsDeviceName(sceneName);
}
