#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace synaptics {
namespace synap {

/// Zip extraction tool.
class ZipTool
{
public:
    ZipTool();
    ~ZipTool();

    /// Open Zip archive from file.
    bool open(std::string zipFileName);
    /// Open Zip archive from data in memory.
    bool open(const void *zip_data, size_t size);
    /// close the Zip archive.
    bool close();

    /// Extract Archive from name.
    /// \retval empty vector if not found
    std::vector<uint8_t> extract_archive(const std::string & name);

    /// Extract all archives into passed directory.
    bool extract_all(const std::string & out_directory);

    /// Archive descriptor
    struct Archive {
        uint32_t index; ///< Internal index of the archive
        std::string name; ///< Name/path of the archive
        size_t size; ///< Size of the archive
    };

    /// Get archive list for independent archive extraction
    /// \warning This only list files, not directory so empty directories will be ignored.
    std::vector<Archive> get_archive_list() const;

    /// Extract Archive from index
    /// \warning \param archiveData must point to a memory area with enough space
    /// \sa Archive::size
    bool extract_archive(uint32_t index, uint8_t * archiveData);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

}  // namespace synap
}  // namespace synaptics
