#pragma once

#include <filesystem>

// Owns a sibling temporary file for a single output path. Until Commit() is
// called, failure only removes that temporary file; it never changes the
// existing destination ESM.
class AtomicOutputFile final {
public:
    explicit AtomicOutputFile(std::filesystem::path finalPath);
    ~AtomicOutputFile() noexcept;

    AtomicOutputFile(const AtomicOutputFile&) = delete;
    AtomicOutputFile& operator=(const AtomicOutputFile&) = delete;

    [[nodiscard]] const std::filesystem::path& TemporaryPath() const noexcept;
    void Commit();

private:
    std::filesystem::path finalPath_;
    std::filesystem::path temporaryPath_;
    bool committed_ = false;
};