#include "AtomicOutputFile.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <system_error>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace {

[[nodiscard]] std::filesystem::path MakeTemporaryPath(const std::filesystem::path& finalPath) {
    if (finalPath.filename().empty()) {
        throw std::invalid_argument("Atomic output requires a file name.");
    }

    static std::atomic<std::uint64_t> sequence{0};
    const auto now = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const std::uint64_t randomPart =
        (static_cast<std::uint64_t>(std::random_device{}()) << 32U) ^ std::random_device{}();

    for (std::uint32_t attempt = 0; attempt < 128; ++attempt) {
        const std::uint64_t suffix = now ^ randomPart ^ sequence.fetch_add(1, std::memory_order_relaxed);
        const std::filesystem::path candidate = finalPath.parent_path() /
            (finalPath.filename().string() + ".writing-" + std::to_string(suffix) + ".tmp");

        std::error_code error;
        const bool alreadyExists = std::filesystem::exists(candidate, error);
        if (error) {
            throw std::system_error(error, "Unable to inspect the temporary ESM path");
        }
        if (!alreadyExists) {
            return candidate;
        }
    }

    throw std::runtime_error("Unable to reserve a unique temporary output path for the ESM.");
}

} // namespace

AtomicOutputFile::AtomicOutputFile(std::filesystem::path finalPath)
    : finalPath_(std::move(finalPath))
    , temporaryPath_(MakeTemporaryPath(finalPath_)) {}

AtomicOutputFile::~AtomicOutputFile() noexcept {
    if (!committed_) {
        std::error_code ignored;
        std::filesystem::remove(temporaryPath_, ignored);
    }
}

const std::filesystem::path& AtomicOutputFile::TemporaryPath() const noexcept {
    return temporaryPath_;
}

void AtomicOutputFile::Commit() {
    if (committed_) {
        throw std::logic_error("Atomic output was committed more than once.");
    }

    std::error_code existsError;
    const bool finalExists = std::filesystem::exists(finalPath_, existsError);
    if (existsError) {
        throw std::system_error(existsError, "Unable to inspect the final ESM path");
    }

#ifdef _WIN32
    if (finalExists) {
        if (!::ReplaceFileW(
                finalPath_.c_str(),
                temporaryPath_.c_str(),
                nullptr,
                REPLACEFILE_WRITE_THROUGH,
                nullptr,
                nullptr)) {
            throw std::system_error(
                static_cast<int>(::GetLastError()),
                std::system_category(),
                "Unable to atomically replace the existing ESM output");
        }
    } else if (!::MoveFileExW(
                   temporaryPath_.c_str(),
                   finalPath_.c_str(),
                   MOVEFILE_WRITE_THROUGH)) {
        throw std::system_error(
            static_cast<int>(::GetLastError()),
            std::system_category(),
            "Unable to publish the completed ESM output");
    }
#else
    // POSIX rename replaces the destination atomically when both paths are on
    // the same filesystem. The temporary path is intentionally a sibling.
    std::filesystem::rename(temporaryPath_, finalPath_);
#endif

    committed_ = true;
}